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
#include "api.h"
#include "ptx_logging.h"

#define PTX_IGNITION_DURATION_MS   5000U
#define PTX_PERIODIC_LOG_MS        1000U
#define PTX_VREF_MIN               4.5f
#define PTX_VREF_MAX               5.5f
#define PTX_DOOR_OPEN_THRESHOLD_MV 2500U   /* legacy analog threshold (not used in interrupt mode) */

/* Output control macros for clarity */
#define PTX_GAS_ON()         set_output(GAS_VALVE, 1)
#define PTX_GAS_OFF()        set_output(GAS_VALVE, 0)
#define PTX_IGNITER_ON()     set_output(IGNITER, 1)
#define PTX_IGNITER_OFF()    set_output(IGNITER, 0)
/* Set-by-state macros to avoid if/else branching at call sites */
#define PTX_SET_GAS(state)       set_output(GAS_VALVE,   ((state) ? 1 : 0))
#define PTX_SET_IGNITER(state)   set_output(IGNITER, ((state) ? 1 : 0))

/* Internal state */
static ptx_oven_status_t pti_status;
static uint32_t pti_ignition_start_ms = 0;
static uint32_t pti_last_log_ms = 0;

static bool ptx_read_door_open(void) {
    /* Door state now provided by external interrupt handler via setter. */
    return pti_status.door_open;
}

static void ptx_eval_sensor_faults(float vref_mv, float signal_mv) {
    pti_status.vref_volts   = vref_mv / 1000.0f;
    pti_status.signal_volts = signal_mv / 1000.0f;

    pti_status.vref_fault   = (pti_status.vref_volts < PTX_VREF_MIN) || (pti_status.vref_volts > PTX_VREF_MAX);

    /* Signal valid only if within [10%, 90%] of vref (inclusive). */
    float lo = 0.10f * vref_mv;
    float hi = 0.90f * vref_mv;
    pti_status.signal_fault = (signal_mv < lo) || (signal_mv > hi);

    pti_status.sensor_fault = pti_status.vref_fault || pti_status.signal_fault;
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
    PTX_SET_GAS(pti_status.gas_on);
    PTX_SET_IGNITER(pti_status.igniter_on);

    /* Optional LED debug (guard with your own defines to avoid build errors)
       Example:
    // set_output(LED_STATUS, pti_status.sensor_fault ? 1 : 0);
    */
}

static void ptx_update_heating(uint32_t now_ms) {
    /* Door and sensor faults override everything. */
    if (pti_status.door_open || pti_status.sensor_fault) {
        if (pti_status.gas_on || pti_status.igniter_on) {
            PTX_LOGF("shutdown: door open or sensor fault");
        }
        pti_status.gas_on = false;
        pti_status.igniter_on = false;
        pti_status.state = PTX_HEATING_STATE_IDLE;
        return;
    }

    /* Hysteresis control near expected temperature. */
    if (pti_status.gas_on) {
        /* Heating: turn off when reaching upper threshold. */
        if (pti_status.temperature_c >= PTX_TEMP_OFF_C) {
            pti_status.gas_on = false;
            pti_status.igniter_on = false;
            pti_status.state = PTX_HEATING_STATE_IDLE;
            int temp_c_i = (int)(pti_status.temperature_c + 0.5f);
            PTX_LOGF("heat off temp=%dC", temp_c_i);
            return;
        }

        /* Manage ignition timeout. */
        if (pti_status.state == PTX_HEATING_STATE_IGNITING) {
            if ((now_ms - pti_ignition_start_ms) >= PTX_IGNITION_DURATION_MS) {
                pti_status.igniter_on = false;
                pti_status.state = PTX_HEATING_STATE_HEATING; /* flame expected */
                PTX_LOGF("ignite complete");
            }
        }
        /* Else keep heating with gas ON (flame expected). */
    } else {
        /* Idle: start heating when below lower threshold. */
        if (pti_status.temperature_c <= PTX_TEMP_ON_C) {
            pti_status.gas_on = true;                 /* open gas valve */
            pti_status.igniter_on = true;             /* turn on igniter */
            pti_status.state = PTX_HEATING_STATE_IGNITING;
            pti_ignition_start_ms = now_ms;
            int temp_c_i = (int)(pti_status.temperature_c + 0.5f);
            PTX_LOGF("ignite start temp=%dC", temp_c_i);
        }
    }
}

static void ptx_maybe_log(uint32_t now_ms) {
    if ((now_ms - pti_last_log_ms) < PTX_PERIODIC_LOG_MS) return;
    pti_last_log_ms = now_ms;

    int vref_mV = (int)(pti_status.vref_volts * 1000.0f + 0.5f);
    int signal_mV = (int)(pti_status.signal_volts * 1000.0f + 0.5f);
    int temp_c_i = (int)(pti_status.temperature_c + 0.5f);
    PTX_LOGF("vref=%dmV signal=%dmV temp=%dC door=%s state=%d gas=%d igniter=%d vref_fault=%d signal_fault=%d",
             vref_mV,
             signal_mV,
             temp_c_i,
             pti_status.door_open ? "OPEN" : "CLOSED",
             (int)pti_status.state,
             pti_status.gas_on ? 1 : 0,
             pti_status.igniter_on ? 1 : 0,
             pti_status.vref_fault ? 1 : 0,
             pti_status.signal_fault ? 1 : 0);
}

/* Public API */
const ptx_oven_status_t* ptx_get_oven_status(void) {
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

    pti_ignition_start_ms = 0;
    pti_last_log_ms = 0;

    PTX_LOGF("oven control init");
}

void ptx_oven_control_update(void) {
    uint32_t now = millis();

    /* Read raw millivolts from API. */
    float vref_mv   = (float)read_voltage(TEMPERATURE_SENSOR_REFERENCE);
    float signal_mv = (float)read_voltage(TEMPERATURE_SENSOR);

    /* Evaluate faults first. */
    ptx_eval_sensor_faults(vref_mv, signal_mv);
    pti_status.door_open = ptx_read_door_open();

    /* Compute temperature (for display/log); control will still be overridden on faults. */
    pti_status.temperature_c = ptx_compute_temperature(vref_mv, signal_mv);

    /* Control decision. */
    ptx_update_heating(now);

    /* Apply outputs and log. */
    ptx_apply_outputs();
    ptx_maybe_log(now);
}

void ptx_oven_set_door_state(bool open) {
    pti_status.door_open = open;
}