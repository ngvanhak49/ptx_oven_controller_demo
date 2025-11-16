/**
 * @file ptx_actuator.cpp
 * @brief Implementation of actuator control layer
 */
#include "ptx_actuator.h"
#include "api.h"

void ptx_actuator_init(void) {
    /* Start with all actuators OFF for safety */
    set_output(GAS_VALVE, 0);
    set_output(IGNITER, 0);
}

void ptx_actuator_set_gas(bool enable) {
    set_output(GAS_VALVE, enable ? 1 : 0);
}

void ptx_actuator_set_igniter(bool enable) {
    set_output(IGNITER, enable ? 1 : 0);
}

void ptx_actuator_emergency_stop(void) {
    set_output(GAS_VALVE, 0);
    set_output(IGNITER, 0);
}

bool ptx_actuator_get_gas_state(void) {
    return read_output(GAS_VALVE);
}

bool ptx_actuator_get_igniter_state(void) {
    return read_output(IGNITER);
}
