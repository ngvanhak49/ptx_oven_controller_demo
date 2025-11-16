#include <stdio.h>
#include <math.h>
#include "ptx_oven_control.h"
#include "ptx_oven_control.h"
#include "tests/mocks/mock_api.h"

#define ASSERT_TRUE(msg, cond) do { if(!(cond)) { printf("ASSERT_TRUE failed: %s\n", msg); return 1; } } while(0)
#define ASSERT_FALSE(msg, cond) ASSERT_TRUE(msg, !(cond))
#define ASSERT_EQ_INT(msg, a, b) do { if((int)(a)!=(int)(b)) { printf("ASSERT_EQ failed: %s (%d != %d)\n", msg, (int)(a), (int)(b)); return 1; } } while(0)

static uint16_t mv_for_temp(float vref_mv, float temp_c) {
    // Inverse of mapping in ptx_compute_temperature
    float val = ((temp_c + 10.0f) / 310.0f) * (0.80f * vref_mv) + 0.10f * vref_mv;
    return (uint16_t)(val + 0.5f);
}

static int test_door_open_shutdown() {
    mock_reset_time(0);
    ptx_oven_control_init();
    ptx_oven_set_door_state(false);
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f)); // below ON threshold

    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_get_oven_status();
    ASSERT_TRUE("gas should start ON", st->gas_on);
    ASSERT_TRUE("igniter should start ON", st->igniter_on);

    // Open door -> immediate shutdown on next update
    ptx_oven_set_door_state(true);
    ptx_oven_control_update();
    st = ptx_get_oven_status();
    ASSERT_FALSE("gas off on door open", st->gas_on);
    ASSERT_FALSE("igniter off on door open", st->igniter_on);
    ASSERT_FALSE("output gas off", mock_get_gas_output());
    ASSERT_FALSE("output igniter off", mock_get_igniter_output());
    return 0;
}

static int test_ignition_timing() {
    mock_reset_time(0);
    ptx_oven_control_init();
    ptx_oven_set_door_state(false);
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_get_oven_status();
    ASSERT_TRUE("igniter ON during ignition", st->igniter_on);

    mock_advance_ms(5000);
    ptx_oven_control_update();
    st = ptx_get_oven_status();
    ASSERT_TRUE("gas stays ON after ignition", st->gas_on);
    ASSERT_FALSE("igniter OFF after 5s", st->igniter_on);
    return 0;
}

static int test_hysteresis_turn_off() {
    mock_reset_time(0);
    ptx_oven_control_init();
    ptx_oven_set_door_state(false);
    mock_set_vref_mv(5000);

    // Start heating (below ON threshold)
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));
    ptx_oven_control_update();

    // Move above OFF threshold
    mock_set_signal_mv(mv_for_temp(5000, 185.0f));
    ptx_oven_control_update();

    const ptx_oven_status_t* st = ptx_get_oven_status();
    ASSERT_FALSE("gas OFF above OFF threshold", st->gas_on);
    ASSERT_FALSE("igniter OFF above OFF threshold", st->igniter_on);
    return 0;
}

static int test_sensor_fault_vref_range() {
    mock_reset_time(0);
    ptx_oven_control_init();
    ptx_oven_set_door_state(false);
    mock_set_vref_mv(4000); // below 4.5V
    mock_set_signal_mv(mv_for_temp(4000, 160.0f));

    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_get_oven_status();
    ASSERT_TRUE("sensor fault set", st->sensor_fault);
    ASSERT_FALSE("gas OFF on sensor fault", st->gas_on);
    ASSERT_FALSE("igniter OFF on sensor fault", st->igniter_on);
    return 0;
}

int main() {
    int rc = 0;
    rc |= test_door_open_shutdown();
    rc |= test_ignition_timing();
    rc |= test_hysteresis_turn_off();
    rc |= test_sensor_fault_vref_range();
    if (rc == 0) {
        printf("All tests passed.\n");
    }
    return rc;
}
