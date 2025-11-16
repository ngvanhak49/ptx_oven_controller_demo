/**
 * @file ptx_oven_control.h
 * @brief Oven control in C-style: 180 °C target, door safety, ignition timing, sensor validation.
 * @details Implements a simple bang-bang controller with hysteresis around a target temperature.
 *          Door open condition and sensor faults override all heating actions immediately.
 */
#ifndef PTX_OVEN_CONTROL_H
#define PTX_OVEN_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Update debounced door state from external interrupt handler.
 * @param open true if door is open, false if closed.
 */
void ptx_oven_set_door_state(bool open);

/**
 * @brief Heating state machine for the oven.
 */
typedef enum {
    PTX_HEATING_STATE_IDLE = 0,   /**< Outputs off; waiting for heat demand. */
    PTX_HEATING_STATE_IGNITING,   /**< First 5 seconds after gas turns on (igniter ON). */
    PTX_HEATING_STATE_HEATING     /**< Post-ignition; flame expected; igniter OFF. */
} ptx_heating_state_t;

/**
 * @brief Public status snapshot of the oven control loop.
 */
typedef struct {
    float vref_volts;          /**< Reference voltage from sensor (V). */
    float signal_volts;        /**< Sensor signal (V), referenced to vref. */
    float temperature_c;       /**< Computed temperature (°C). */
    bool  door_open;           /**< Door state: true=open, false=closed. */
    bool  gas_on;              /**< Gas valve command output. */
    bool  igniter_on;          /**< Igniter command output. */
    ptx_heating_state_t state; /**< Current heating state. */

    /* Faults */
    bool  vref_fault;          /**< True if vref not in [4.5, 5.5] V. */
    bool  signal_fault;        /**< True if signal not in [10%, 90%] of vref. */
    bool  sensor_fault;        /**< Aggregate: vref_fault || signal_fault. */
} ptx_oven_status_t;

/**
 * @brief Initialize oven control module.
 * @note Does not configure hardware I/O; relies on api.h setup.
 */
void ptx_oven_control_init(void);
/**
 * @brief Execute one control loop iteration.
 * @details Reads inputs, validates sensors, updates heating state, and drives outputs.
 */
void ptx_oven_control_update(void);
/**
 * @brief Get a pointer to the latest status snapshot.
 * @return Pointer to constant ptx_oven_status_t structure.
 */
const ptx_oven_status_t* ptx_get_oven_status(void);

/* Tuning (hysteresis around target) */
/** @brief Target temperature (°C). */
#define PTX_TEMP_TARGET_C   (180.0f)
/** @brief Hysteresis half-band (°C). */
#define PTX_TEMP_DELTA_C    (2.0f)
/** @brief Start heating below (target - delta). */
#define PTX_TEMP_ON_C       (PTX_TEMP_TARGET_C - PTX_TEMP_DELTA_C)
/** @brief Stop heating at/above (target + delta). */
#define PTX_TEMP_OFF_C      (PTX_TEMP_TARGET_C + PTX_TEMP_DELTA_C)

#endif /* PTX_OVEN_CONTROL_H */