/**
 * @file ptx_actuator.h
 * @brief Actuator control abstraction layer (gas valve, igniter)
 * @details Provides clean interface between application logic and hardware API
 */
#ifndef PTX_ACTUATOR_H
#define PTX_ACTUATOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize actuator outputs
 * @note Ensures all actuators start in safe state (OFF)
 */
void ptx_actuator_init(void);

/**
 * @brief Control gas valve state
 * @param enable true to open valve, false to close
 */
void ptx_actuator_set_gas(bool enable);

/**
 * @brief Control igniter state
 * @param enable true to activate igniter, false to deactivate
 */
void ptx_actuator_set_igniter(bool enable);

/**
 * @brief Emergency shutdown - turn off all actuators immediately
 * @note Used for safety cutoff (door open, sensor fault, etc.)
 */
void ptx_actuator_emergency_stop(void);

/**
 * @brief Get current gas valve state
 * @return true if gas valve is open, false if closed
 */
bool ptx_actuator_get_gas_state(void);

/**
 * @brief Get current igniter state
 * @return true if igniter is active, false if inactive
 */
bool ptx_actuator_get_igniter_state(void);

#ifdef __cplusplus
}
#endif

#endif /* PTX_ACTUATOR_H */
