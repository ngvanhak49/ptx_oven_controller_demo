/**
 * @file ptx_oven_control.cpp
 * @brief Control logic per requirements:
 *        - Maintain near 180C using hysteresis (ON at 178C, OFF at 182C).
 *        - Door open overrides everything -> gas OFF, igniter OFF immediately.
 *        - Igniter ON only first 5s after gas turns ON.
 *        - vref must be 4.5–5.5V; signal must be within 10–90% vref; else fault -> shutdown.
 *        - Periodically log vref, signal, computed temperature, and state.
 */
#include "ptx_oven_control.h"
#include "ptx_oven_config.h"
#include "ptx_sensor_filter.h"
#include "ptx_actuator.h"
#include "api.h"
#include "ptx_logging.h"

/* Feature flags */
#ifndef PTX_FLAME_DETECT_ENABLED
#define PTX_FLAME_DETECT_ENABLED 0  /* Disable flame detection by default (assume ignition success) */
#endif

/* Internal state */
static ptx_oven_status_t pti_status;
static uint32_t pti_ignition_start_ms = 0;
static uint32_t pti_last_log_ms = 0;

/* Timed sensor fault management */
static uint32_t pti_out_of_range_since_ms = 0;   /* 0 means not currently out of range */
static uint32_t pti_valid_since_ms = 0;          /* 0 means not in continuous valid window */

/* Ignition retry management */
static uint8_t pti_ignition_attempt = 0;         /* Current attempt number (0 = not started) */
static uint32_t pti_purge_start_ms = 0;          /* Start time of purge phase */
static float pti_temp_at_ignition_start = 0.0f;  /* Temperature when ignition started (for flame detection) */

static bool ptx_read_door_open(void) {
    return pti_status.door_open;
}

static void ptx_eval_sensor_faults_with_timing(uint32_t now_ms, float vref_mv, float signal_mv) {
    const ptx_oven_config_t* cfg = ptx_oven_get_config();
    
    /* Update instantaneous readings */
    pti_status.vref_volts   = vref_mv / 1000.0f;
    pti_status.signal_volts = signal_mv / 1000.0f;

    /* Instantaneous violations (not latched) */
    bool vref_bad = (pti_status.vref_volts < cfg->vref_min_v) || (pti_status.vref_volts > cfg->vref_max_v);

    float lo = 0.10f * vref_mv;
    float hi = 0.90f * vref_mv;
    bool signal_bad = (signal_mv < lo) || (signal_mv > hi);

    pti_status.vref_fault = vref_bad;        /* expose instantaneous state */
    pti_status.signal_fault = signal_bad;

    bool out_of_range = vref_bad || signal_bad;

    if (out_of_range) {
        /* Reset valid window and start/continue out-of-range window */
        pti_valid_since_ms = 0;
        if (pti_out_of_range_since_ms == 0) {
            pti_out_of_range_since_ms = now_ms;
        }
        /* Latch fault only if persists beyond window */
        if (!pti_status.sensor_fault && (now_ms - pti_out_of_range_since_ms) > cfg->sensor_fault_window_ms) {
            pti_status.sensor_fault = true;
            PTX_LOGF("sensor fault latched");
        }
    } else {
        /* Readings are valid; clear out-of-range window */
        pti_out_of_range_since_ms = 0;
        if (pti_status.sensor_fault) {
            /* If fault was latched, require continuous validity before auto-resume */
            if (pti_valid_since_ms == 0) {
                pti_valid_since_ms = now_ms;
            }
            if ((now_ms - pti_valid_since_ms) >= cfg->auto_resume_delay_ms) {
                pti_status.sensor_fault = false; /* clear latched fault */
                pti_valid_since_ms = 0;
                PTX_LOGF("sensor fault cleared");
            }
        } else {
            /* No latched fault; keep valid_since reset */
            pti_valid_since_ms = 0;
        }
    }
}

static float ptx_compute_temperature(float vref_mv, float signal_mv) {
    /* Linear map -10C at 10% vref to 300C at 90% vref (span 310C over 0.8*vref). */
    float low = 0.10f * vref_mv;
    float high = 0.90f * vref_mv;

    if (signal_mv <= low) return -10.0f;
    if (signal_mv >= high) return 300.0f;

    return -10.0f + ((signal_mv - low) / (0.80f * vref_mv)) * 310.0f;
}

static void ptx_apply_outputs(void) {
    ptx_actuator_set_gas(pti_status.gas_on);
    ptx_actuator_set_igniter(pti_status.igniter_on);

    /* Optional LED debug (guard with your own defines to avoid build errors)
       Example:
    // set_output(LED_STATUS, pti_status.sensor_fault ? 1 : 0);
    */
}

static void ptx_update_heating(uint32_t now_ms) {
    const ptx_oven_config_t* cfg = ptx_oven_get_config();
    
    /* Check if in lockout state */
    if (pti_status.state == PTX_HEATING_STATE_LOCKOUT) {
        pti_status.gas_on = false;
        pti_status.igniter_on = false;
        pti_status.ignition_lockout = true;
        return; /* Require manual reset */
    }
    
    /* Door and sensor faults override everything. */
    if (pti_status.door_open || pti_status.sensor_fault) {
        if (pti_status.gas_on || pti_status.igniter_on) {
            PTX_LOGF("shutdown: door open or sensor fault");
        }
        pti_status.gas_on = false;
        pti_status.igniter_on = false;
        pti_status.state = PTX_HEATING_STATE_IDLE;
        pti_ignition_attempt = 0; /* Reset attempt counter on fault */
        return;
    }

    /* Handle purge state (after failed ignition) */
    if (pti_status.state == PTX_HEATING_STATE_PURGING) {
        if ((now_ms - pti_purge_start_ms) >= cfg->purge_time_ms) {
            /* Purge complete, ready to retry */
            pti_status.state = PTX_HEATING_STATE_IDLE;
            PTX_LOGF("purge complete, attempt=%d", pti_ignition_attempt);
        }
        return;
    }

    /* Hysteresis control near expected temperature. */
    float temp_on = cfg->temp_target_c - cfg->temp_delta_c;
    float temp_off = cfg->temp_target_c + cfg->temp_delta_c;
    
    if (pti_status.gas_on) {
        /* Heating: turn off when reaching upper threshold. */
        if (pti_status.temperature_c >= temp_off) {
            pti_status.gas_on = false;
            pti_status.igniter_on = false;
            pti_status.state = PTX_HEATING_STATE_IDLE;
            pti_ignition_attempt = 0; /* Successful heating, reset counter */
            int temp_c_i = (int)(pti_status.temperature_c + 0.5f);
            PTX_LOGF("heat off temp=%dC", temp_c_i);
            return;
        }

        /* Manage ignition phase. */
        if (pti_status.state == PTX_HEATING_STATE_IGNITING) {
            if ((now_ms - pti_ignition_start_ms) >= cfg->ignition_duration_ms) {
                /* Ignition period ended, check for flame (temperature rise) */
                float temp_rise = pti_status.temperature_c - pti_temp_at_ignition_start;

#if (PTX_FLAME_DETECT_ENABLED)
                if (temp_rise > 5.0f) {
                    /* Flame detected (temp increased), successful ignition */
                    pti_status.igniter_on = false;
                    pti_status.state = PTX_HEATING_STATE_HEATING;
                    pti_ignition_attempt = 0; /* Success, reset counter */
                    PTX_LOGF("ignition success, temp_rise=%dC", (int)temp_rise);
                } else {
                    /* Failed ignition (no temp rise) */
                    pti_status.gas_on = false;
                    pti_status.igniter_on = false;
                    
                    if (pti_ignition_attempt >= cfg->max_ignition_attempts) {
                        /* Max attempts reached, enter lockout */
                        pti_status.state = PTX_HEATING_STATE_LOCKOUT;
                        pti_status.ignition_lockout = true;
                        PTX_LOGF("ignition lockout after %d attempts", pti_ignition_attempt);
                    } else {
                        /* Start purge before retry */
                        pti_status.state = PTX_HEATING_STATE_PURGING;
                        pti_purge_start_ms = now_ms;
                        PTX_LOGF("ignition failed attempt=%d, purging", pti_ignition_attempt);
                    }
                }
#else
                /* Flame detection disabled; assume success */
                pti_status.igniter_on = false;
                pti_status.state = PTX_HEATING_STATE_HEATING;
                pti_ignition_attempt = 0; /* Success, reset counter */
                PTX_LOGF("ignition assumed success (flame detect disabled)");
#endif
            }
        }
        /* Else keep heating with gas ON (flame expected). */
    } else {
        /* Idle: start heating when below lower threshold. */
        if (pti_status.temperature_c <= temp_on) {
            pti_ignition_attempt++;                   /* Increment attempt counter */
            pti_status.gas_on = true;                 /* open gas valve */
            pti_status.igniter_on = true;             /* turn on igniter */
            pti_status.state = PTX_HEATING_STATE_IGNITING;
            pti_ignition_start_ms = now_ms;
            pti_temp_at_ignition_start = pti_status.temperature_c; /* Record starting temp */
            int temp_c_i = (int)(pti_status.temperature_c + 0.5f);
            PTX_LOGF("ignite start attempt=%d temp=%dC", pti_ignition_attempt, temp_c_i);
        }
    }
}

static void ptx_oven_run_log(uint32_t now_ms) {
    const ptx_oven_config_t* cfg = ptx_oven_get_config();
    
    if ((now_ms - pti_last_log_ms) < cfg->periodic_log_ms) return;
    pti_last_log_ms = now_ms;

    int vref_mV = (int)(pti_status.vref_volts * 1000.0f + 0.5f);
    int signal_mV = (int)(pti_status.signal_volts * 1000.0f + 0.5f);
    int temp_c_i = (int)(pti_status.temperature_c + 0.5f);
    PTX_LOGF("vref=%dmV signal=%dmV temp=%dC door=%s state=%d gas=%d ign=%d attempt=%d lockout=%d",
             vref_mV,
             signal_mV,
             temp_c_i,
             pti_status.door_open ? "OPEN" : "CLOSED",
             (int)pti_status.state,
             pti_status.gas_on ? 1 : 0,
             pti_status.igniter_on ? 1 : 0,
             pti_status.ignition_attempt,
             pti_status.ignition_lockout ? 1 : 0);
}

/* Public API */
const ptx_oven_status_t* ptx_oven_get_status(void) {
    return &pti_status;
}

void ptx_oven_control_init(void) {
    pti_status.vref_volts = 0.0f;
    pti_status.signal_volts = 0.0f;
    pti_status.temperature_c = -10.0f;
    pti_status.door_open = false;
    pti_status.gas_on = false;
    pti_status.igniter_on = false;
    pti_status.state = PTX_HEATING_STATE_IDLE;
    pti_status.vref_fault = false;
    pti_status.signal_fault = false;
    pti_status.sensor_fault = false;
    pti_status.ignition_attempt = 0;
    pti_status.ignition_lockout = false;

    pti_ignition_start_ms = 0;
    pti_last_log_ms = 0;
    pti_ignition_attempt = 0;
    pti_purge_start_ms = 0;
    pti_temp_at_ignition_start = 0.0f;
    
    /* Initialize actuators and sensor filter */
    ptx_actuator_init();
    ptx_sensor_filter_init(5);

    PTX_LOGF("oven control init");
}

void ptx_oven_control_update(void) {
    uint32_t now = millis();

    /* Read and filter sensor data */
    ptx_sensor_reading_t filtered = ptx_sensor_filter_read_and_update();
    
    float vref_mv   = (float)filtered.vref_mv;
    float signal_mv = (float)filtered.signal_mv;

    /* Evaluate faults with timing first. */
    ptx_eval_sensor_faults_with_timing(now, vref_mv, signal_mv);
    pti_status.door_open = ptx_read_door_open();

    /* Compute temperature (for display/log); control will still be overridden on faults. */
    pti_status.temperature_c = ptx_compute_temperature(vref_mv, signal_mv);

    /* Control decision. */
    ptx_update_heating(now);

    /* Apply outputs and log. */
    ptx_apply_outputs();
    ptx_oven_run_log(now);
    
    /* Update public status */
    pti_status.ignition_attempt = pti_ignition_attempt;
}

void ptx_oven_set_door_state(bool open) {
    pti_status.door_open = open;
}

void ptx_oven_reset_ignition_lockout(void) {
    if (pti_status.state == PTX_HEATING_STATE_LOCKOUT) {
        pti_status.state = PTX_HEATING_STATE_IDLE;
        pti_status.ignition_lockout = false;
        pti_ignition_attempt = 0;
        pti_status.ignition_attempt = 0;
        PTX_LOGF("ignition lockout reset");
    }
}