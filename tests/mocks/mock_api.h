#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Control fake time
void mock_reset_time(unsigned long now_ms);
void mock_advance_ms(unsigned long delta_ms);

// Control analog inputs (millivolts)
void mock_set_vref_mv(uint16_t mv);
void mock_set_signal_mv(uint16_t mv);

// Inspect outputs
bool mock_get_gas_output(void);
bool mock_get_igniter_output(void);

#ifdef __cplusplus
}
#endif
