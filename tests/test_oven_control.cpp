#include <stdio.h>
#include <math.h>
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
    const ptx_oven_status_t* st = ptx_oven_get_status();
    ASSERT_TRUE("gas should start ON", st->gas_on);
    ASSERT_TRUE("igniter should start ON", st->igniter_on);

    // Open door -> immediate shutdown on next update
    ptx_oven_set_door_state(true);
    ptx_oven_control_update();
    st = ptx_oven_get_status();
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
    const ptx_oven_status_t* st = ptx_oven_get_status();
    ASSERT_TRUE("igniter ON during ignition", st->igniter_on);

    mock_advance_ms(5000);
    ptx_oven_control_update();
    st = ptx_oven_get_status();
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
    
    // Fill median filter buffer first
    for (int i = 0; i < 5; ++i) {
        ptx_oven_control_update();
        mock_advance_ms(50);
    }

    // Wait for ignition to complete
    mock_advance_ms(5000);
    ptx_oven_control_update();

    // Move above OFF threshold - need to replace all values in filter
    mock_set_signal_mv(mv_for_temp(5000, 186.0f));
    for (int i = 0; i < 15; ++i) {
        mock_advance_ms(50);
        ptx_oven_control_update();
    }

    const ptx_oven_status_t* st = ptx_oven_get_status();
    ASSERT_FALSE("gas OFF above OFF threshold", st->gas_on);
    ASSERT_FALSE("igniter OFF above OFF threshold", st->igniter_on);
    return 0;
}

static int test_sensor_fault_vref_range() {
    mock_reset_time(0);
    ptx_oven_control_init();
    ptx_oven_set_door_state(false);
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    // Start heating
    ptx_oven_control_update();

    // Now make vref invalid and hold for >1s
    mock_set_vref_mv(4000); // below 4.5V
    ptx_oven_control_update(); // first update with bad vref starts timer at t=0
    for (int i = 0; i < 12; ++i) { // 12 * 100ms = 1200ms to ensure > 1000ms elapsed
        mock_advance_ms(100);
        ptx_oven_control_update();
    }

    const ptx_oven_status_t* st = ptx_oven_get_status();
    ASSERT_TRUE("sensor fault latched after 1s", st->sensor_fault);
    ASSERT_FALSE("gas OFF on sensor fault", st->gas_on);
    ASSERT_FALSE("igniter OFF on sensor fault", st->igniter_on);
    return 0;
}

static int test_auto_resume_after_valid_window() {
    mock_reset_time(0);
    ptx_oven_control_init();
    ptx_oven_set_door_state(false);
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    // Start heating and complete ignition
    ptx_oven_control_update();
    mock_advance_ms(5000); // Wait for ignition to complete
    ptx_oven_control_update();

    // Trigger sensor fault by invalid vref for >1s
    mock_set_vref_mv(4000);
    ptx_oven_control_update(); // starts timer
    for (int i = 0; i < 12; ++i) { // 1200ms elapsed > 1000ms threshold
        mock_advance_ms(100);
        ptx_oven_control_update();
    }

    // Restore valid vref and wait 3s valid window
    mock_set_vref_mv(5000);
    ptx_oven_control_update(); // starts valid timer at current time
    for (int i = 0; i < 35; ++i) { // 35 * 100ms = 3500ms > 3000ms threshold (extra margin)
        mock_advance_ms(100);
        ptx_oven_control_update();
    }

    const ptx_oven_status_t* st = ptx_oven_get_status();
    ASSERT_FALSE("sensor fault cleared after 3s valid", st->sensor_fault);
    // Since temperature is below ON threshold, controller should auto-reignite
    ASSERT_TRUE("gas ON after resume", st->gas_on);
    ASSERT_TRUE("igniter ON at ignite start after resume", st->igniter_on);
    return 0;
}

int main() {
    int rc = 0;
    rc |= test_door_open_shutdown();
    rc |= test_ignition_timing();
    rc |= test_hysteresis_turn_off();
    rc |= test_sensor_fault_vref_range();
    rc |= test_auto_resume_after_valid_window();
    if (rc == 0) {
        printf("All tests passed.\n");
    }
    return rc;
}
