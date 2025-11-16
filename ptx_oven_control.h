/**
 * @file ptx_oven_control.h
 * @brief Oven control: 180 °C target, door safety, ignition timing, sensor validation.
 * @details Implements a simple bang-bang controller with hysteresis around a target temperature.
 *          Door open condition and sensor faults override all heating actions immediately.
 */
#ifndef PTX_OVEN_CONTROL_H
#define PTX_OVEN_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Update door state from external interrupt handler.
 * @param open true if door is open, false if closed.
 */
void ptx_oven_set_door_state(bool open);

/**
 * @brief Heating state machine for the oven.
 */
typedef enum {
    PTX_HEATING_STATE_IDLE = 0,   /**< Outputs off; waiting for heat demand. */
    PTX_HEATING_STATE_IGNITING,   /**< First 5 seconds after gas turns on (igniter ON). */
    PTX_HEATING_STATE_HEATING,    /**< Post-ignition; flame expected; igniter OFF. */
    PTX_HEATING_STATE_PURGING,    /**< Gas purge after failed ignition before retry. */
    PTX_HEATING_STATE_LOCKOUT     /**< Safety lockout after max failed attempts. */
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

    bool  vref_fault;          /**< True if vref not in [4.5, 5.5] V. */
    bool  signal_fault;        /**< True if signal not in [10%, 90%] of vref. */
    bool  sensor_fault;        /**< Aggregate: vref_fault || signal_fault. */
    
    uint8_t ignition_attempt;  /**< Current ignition attempt counter (1-based). */
    bool    ignition_lockout;  /**< True if in safety lockout after failed ignitions. */
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
const ptx_oven_status_t* ptx_oven_get_status(void);

/**
 * @brief Reset ignition lockout (manual reset after failed attempts)
 * @note Clears lockout state and resets attempt counter
 */
void ptx_oven_reset_ignition_lockout(void);

#ifdef __cplusplus
}
#endif

#endif /* PTX_OVEN_CONTROL_H */