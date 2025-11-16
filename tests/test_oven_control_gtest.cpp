/**
 * @file test_oven_control_gtest.cpp
 * @brief Google Test suite for oven control module
 */
#include <gtest/gtest.h>
#include "ptx_oven_control.h"
#include "tests/mocks/mock_api.h"

// Helper function to convert temperature to sensor millivolt reading
static uint16_t mv_for_temp(float vref_mv, float temp_c) {
    // Inverse of mapping in ptx_compute_temperature
    float val = ((temp_c + 10.0f) / 310.0f) * (0.80f * vref_mv) + 0.10f * vref_mv;
    return (uint16_t)(val + 0.5f);
}

// Test fixture for oven control tests
class OvenControlTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_reset_time(0);
        ptx_oven_control_init();
        ptx_oven_set_door_state(false);
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(OvenControlTest, DoorOpenShutdown) {
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f)); // below ON threshold

    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_TRUE(st->gas_on) << "Gas should start ON when temp below threshold";
    EXPECT_TRUE(st->igniter_on) << "Igniter should start ON";

    // Open door -> immediate shutdown on next update
    ptx_oven_set_door_state(true);
    ptx_oven_control_update();
    st = ptx_oven_get_status();
    EXPECT_FALSE(st->gas_on) << "Gas should turn OFF on door open";
    EXPECT_FALSE(st->igniter_on) << "Igniter should turn OFF on door open";
    EXPECT_FALSE(mock_get_gas_output()) << "Hardware gas output should be OFF";
    EXPECT_FALSE(mock_get_igniter_output()) << "Hardware igniter output should be OFF";
}

TEST_F(OvenControlTest, IgnitionTiming) {
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_TRUE(st->igniter_on) << "Igniter should be ON during ignition phase";

    mock_advance_ms(5000);
    ptx_oven_control_update();
    st = ptx_oven_get_status();
    EXPECT_TRUE(st->gas_on) << "Gas should stay ON after ignition";
    EXPECT_FALSE(st->igniter_on) << "Igniter should turn OFF after 5 seconds";
}

TEST_F(OvenControlTest, HysteresisControl) {
    mock_set_vref_mv(5000);

    // Start heating (below ON threshold: 178°C)
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));
    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_TRUE(st->gas_on) << "Heating should start below ON threshold";

    // Move above OFF threshold (182°C)
    mock_set_signal_mv(mv_for_temp(5000, 185.0f));
    ptx_oven_control_update();
    st = ptx_oven_get_status();
    EXPECT_FALSE(st->gas_on) << "Gas should turn OFF above OFF threshold";
    EXPECT_FALSE(st->igniter_on) << "Igniter should turn OFF above OFF threshold";
}

TEST_F(OvenControlTest, SensorFaultTimedDetection) {
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    // Start heating
    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_TRUE(st->gas_on) << "Should start heating";

    // Make vref invalid and hold for >1s
    mock_set_vref_mv(4000); // below 4.5V threshold
    ptx_oven_control_update(); // first update with bad vref starts timer
    
    for (int i = 0; i < 12; ++i) { // 12 * 100ms = 1200ms > 1000ms threshold
        mock_advance_ms(100);
        ptx_oven_control_update();
    }

    st = ptx_oven_get_status();
    EXPECT_TRUE(st->sensor_fault) << "Sensor fault should latch after 1 second";
    EXPECT_FALSE(st->gas_on) << "Gas should turn OFF on sensor fault";
    EXPECT_FALSE(st->igniter_on) << "Igniter should turn OFF on sensor fault";
}

TEST_F(OvenControlTest, AutoResumeAfterValidWindow) {
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    // Start heating
    ptx_oven_control_update();

    // Trigger sensor fault by invalid vref for >1s
    mock_set_vref_mv(4000);
    ptx_oven_control_update(); // starts fault timer
    
    for (int i = 0; i < 12; ++i) { // 1200ms elapsed > 1000ms threshold
        mock_advance_ms(100);
        ptx_oven_control_update();
    }

    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_TRUE(st->sensor_fault) << "Sensor fault should be latched";

    // Restore valid vref and wait 3s valid window
    mock_set_vref_mv(5000);
    ptx_oven_control_update(); // starts valid timer
    
    for (int i = 0; i < 31; ++i) { // 31 * 100ms = 3100ms > 3000ms threshold
        mock_advance_ms(100);
        ptx_oven_control_update();
    }

    st = ptx_oven_get_status();
    EXPECT_FALSE(st->sensor_fault) << "Sensor fault should clear after 3 seconds of valid readings";
    // Since temperature is below ON threshold, controller should auto-reignite
    EXPECT_TRUE(st->gas_on) << "Gas should turn ON after auto-resume";
    EXPECT_TRUE(st->igniter_on) << "Igniter should turn ON at ignite start after resume";
}

TEST_F(OvenControlTest, IgnitionRetryAfterFailure) {
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f)); // cold start

    // First ignition attempt (no temp rise = failed)
    ptx_oven_control_update();
    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_TRUE(st->gas_on) << "Gas should be ON for first attempt";
    EXPECT_EQ(st->ignition_attempt, 1) << "Should be on attempt 1";

    // Wait for ignition duration (5s) - no temperature rise
    mock_advance_ms(5000);
    ptx_oven_control_update();
    
    st = ptx_oven_get_status();
    EXPECT_FALSE(st->gas_on) << "Gas should turn OFF after failed ignition";
    EXPECT_EQ(st->state, PTX_HEATING_STATE_PURGING) << "Should enter purge state";

    // Wait for purge time (2.5s)
    mock_advance_ms(2500);
    ptx_oven_control_update();
    
    st = ptx_oven_get_status();
    EXPECT_EQ(st->state, PTX_HEATING_STATE_IDLE) << "Should return to IDLE after purge";
}

TEST_F(OvenControlTest, IgnitionLockoutAfterMaxAttempts) {
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    // Simulate 3 failed ignition attempts
    for (int attempt = 1; attempt <= 3; ++attempt) {
        // Trigger heating
        ptx_oven_control_update();
        const ptx_oven_status_t* st = ptx_oven_get_status();
        EXPECT_TRUE(st->gas_on) << "Gas should be ON for attempt " << attempt;
        
        // Wait for ignition duration without temp rise
        mock_advance_ms(5000);
        ptx_oven_control_update();
        
        st = ptx_oven_get_status();
        EXPECT_FALSE(st->gas_on) << "Gas should turn OFF after failed attempt " << attempt;
        
        if (attempt < 3) {
            // Wait for purge
            mock_advance_ms(2500);
            ptx_oven_control_update();
        }
    }

    // After 3rd failure, should be in lockout
    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_EQ(st->state, PTX_HEATING_STATE_LOCKOUT) << "Should enter LOCKOUT after 3 failures";
    EXPECT_TRUE(st->ignition_lockout) << "Lockout flag should be set";
    
    // Try to heat again - should remain locked out
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));
    ptx_oven_control_update();
    st = ptx_oven_get_status();
    EXPECT_FALSE(st->gas_on) << "Should not allow heating in lockout state";
}

TEST_F(OvenControlTest, ManualResetFromLockout) {
    mock_set_vref_mv(5000);
    mock_set_signal_mv(mv_for_temp(5000, 160.0f));

    // Force into lockout state (simplified - directly set state for testing)
    // In real scenario, this would happen after 3 failed attempts
    for (int attempt = 1; attempt <= 3; ++attempt) {
        ptx_oven_control_update();
        mock_advance_ms(5000);
        ptx_oven_control_update();
        if (attempt < 3) {
            mock_advance_ms(2500);
            ptx_oven_control_update();
        }
    }

    const ptx_oven_status_t* st = ptx_oven_get_status();
    EXPECT_EQ(st->state, PTX_HEATING_STATE_LOCKOUT) << "Should be in lockout";

    // Manual reset
    ptx_oven_reset_ignition_lockout();
    
    st = ptx_oven_get_status();
    EXPECT_EQ(st->state, PTX_HEATING_STATE_IDLE) << "Should return to IDLE after reset";
    EXPECT_FALSE(st->ignition_lockout) << "Lockout flag should be cleared";
    EXPECT_EQ(st->ignition_attempt, 0) << "Attempt counter should be reset";
}

// Main function for running all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
