#include <stdint.h>
#include <stdio.h>
#include "api.h"
#include "mock_api.h"

static unsigned long pti_now_ms = 0;
static uint16_t pti_vref_mv = 5000;
static uint16_t pti_signal_mv = 2000;
static bool pti_gas = false;
static bool pti_igniter = false;

extern "C" unsigned long millis(void) {
    return pti_now_ms;
}

extern "C" void mock_reset_time(unsigned long now_ms) { pti_now_ms = now_ms; }
extern "C" void mock_advance_ms(unsigned long delta_ms) { pti_now_ms += delta_ms; }

extern "C" void mock_set_vref_mv(uint16_t mv) { pti_vref_mv = mv; }
extern "C" void mock_set_signal_mv(uint16_t mv) { pti_signal_mv = mv; }

extern "C" uint16_t read_voltage(input_t input) {
    if (input == TEMPERATURE_SENSOR_REFERENCE) return pti_vref_mv;
    if (input == TEMPERATURE_SENSOR) return pti_signal_mv;
    return 0;
}

extern "C" void set_output(output_t output, bool output_state) {
    if (output == GAS_VALVE) pti_gas = output_state;
    else if (output == IGNITER) pti_igniter = output_state;
}

extern "C" uint32_t get_millis() { return (uint32_t)pti_now_ms; }

extern "C" void serial_printf(const char * format, ...) {
    // no-op for tests
}

extern "C" bool mock_get_gas_output(void) { return pti_gas; }
extern "C" bool mock_get_igniter_output(void) { return pti_igniter; }
