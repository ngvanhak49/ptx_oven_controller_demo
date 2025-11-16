/**
 * @file ptx_oven_config.h
 * @brief Configuration parameters for oven controller
 * @details Centralized timing, sensor thresholds, and safety parameters.
 *          All parameters are runtime-configurable via setter functions.
 */
#ifndef PTX_OVEN_CONFIG_H
#define PTX_OVEN_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Oven configuration structure with runtime-adjustable parameters
 */
typedef struct {
    uint32_t ignition_duration_ms;    /**< Duration igniter stays ON after gas opens (default: 5000ms) */
    uint32_t periodic_log_ms;         /**< Interval between periodic status logs (default: 1000ms) */
    uint32_t sensor_fault_window_ms;  /**< Out-of-range duration before latching fault (default: 1000ms) */
    uint32_t auto_resume_delay_ms;    /**< Valid readings duration before clearing fault (default: 3000ms) */
    float    vref_min_v;              /**< Minimum acceptable reference voltage (default: 4.5V) */
    float    vref_max_v;              /**< Maximum acceptable reference voltage (default: 5.5V) */
    float    temp_target_c;           /**< Target temperature for control (default: 180.0°C) */
    float    temp_delta_c;            /**< Hysteresis half-band around target (default: 2.0°C) */
    
    /* Ignition safety parameters */
    uint8_t  max_ignition_attempts;   /**< Maximum number of ignition retry attempts (default: 3) */
    uint32_t purge_time_ms;           /**< Gas purge time after failed ignition (default: 2500ms) */
    float    flame_detect_temp_rise_c; /**< Minimum temperature rise to detect flame (default: 2.0°C) */
} ptx_oven_config_t;

/**
 * @brief Get pointer to current configuration (read-only access)
 * @return Pointer to const configuration structure
 */
const ptx_oven_config_t* ptx_oven_get_config(void);

/**
 * @brief Update oven configuration with new parameters
 * @param config Pointer to new configuration structure
 * @note Changes take effect immediately on next control update
 */
void ptx_oven_set_config(const ptx_oven_config_t* config);

/**
 * @brief Reset configuration to default values
 */
void ptx_oven_reset_config_to_defaults(void);

/**
 * @brief Set ignition duration (milliseconds)
 * @param duration_ms Duration igniter stays ON after gas opens
 */
void ptx_oven_set_ignition_duration_ms(uint32_t duration_ms);

/**
 * @brief Get ignition duration (milliseconds)
 * @return Current ignition duration setting
 */
uint32_t ptx_oven_get_ignition_duration_ms(void);

/**
 * @brief Set periodic log interval (milliseconds)
 * @param interval_ms Interval between status logs
 */
void ptx_oven_set_periodic_log_ms(uint32_t interval_ms);

/**
 * @brief Get periodic log interval (milliseconds)
 * @return Current log interval setting
 */
uint32_t ptx_oven_get_periodic_log_ms(void);

/**
 * @brief Set sensor fault window (milliseconds)
 * @param window_ms Duration out-of-range must persist to latch fault
 */
void ptx_oven_set_sensor_fault_window_ms(uint32_t window_ms);

/**
 * @brief Get sensor fault window (milliseconds)
 * @return Current fault window setting
 */
uint32_t ptx_oven_get_sensor_fault_window_ms(void);

/**
 * @brief Set auto-resume delay (milliseconds)
 * @param delay_ms Duration of valid readings required before clearing fault
 */
void ptx_oven_set_auto_resume_delay_ms(uint32_t delay_ms);

/**
 * @brief Get auto-resume delay (milliseconds)
 * @return Current auto-resume delay setting
 */
uint32_t ptx_oven_get_auto_resume_delay_ms(void);

/**
 * @brief Set reference voltage range (volts)
 * @param min_v Minimum acceptable vref
 * @param max_v Maximum acceptable vref
 */
void ptx_oven_set_vref_range_v(float min_v, float max_v);

/**
 * @brief Get minimum reference voltage (volts)
 * @return Current vref minimum
 */
float ptx_oven_get_vref_min_v(void);

/**
 * @brief Get maximum reference voltage (volts)
 * @return Current vref maximum
 */
float ptx_oven_get_vref_max_v(void);

/**
 * @brief Set target temperature (°C)
 * @param target_c Desired control temperature
 */
void ptx_oven_set_temp_target_c(float target_c);

/**
 * @brief Get target temperature (°C)
 * @return Current target temperature
 */
float ptx_oven_get_temp_target_c(void);

/**
 * @brief Set hysteresis half-band (°C)
 * @param delta_c Half-band around target for ON/OFF thresholds
 */
void ptx_oven_set_temp_delta_c(float delta_c);

/**
 * @brief Get hysteresis half-band (°C)
 * @return Current hysteresis delta
 */
float ptx_oven_get_temp_delta_c(void);

/**
 * @brief Set maximum ignition attempts
 * @param attempts Maximum number of ignition retry attempts (1-5)
 */
void ptx_oven_set_max_ignition_attempts(uint8_t attempts);

/**
 * @brief Get maximum ignition attempts
 * @return Current max ignition attempts
 */
uint8_t ptx_oven_get_max_ignition_attempts(void);

/**
 * @brief Set purge time after failed ignition (milliseconds)
 * @param purge_ms Gas purge duration before retry
 */
void ptx_oven_set_purge_time_ms(uint32_t purge_ms);

/**
 * @brief Get purge time after failed ignition (milliseconds)
 * @return Current purge time
 */
uint32_t ptx_oven_get_purge_time_ms(void);

/**
 * @brief Set flame detection temperature rise threshold (°C)
 * @param temp_rise_c Minimum temperature rise to confirm ignition
 */
void ptx_oven_set_flame_detect_temp_rise_c(float temp_rise_c);

/**
 * @brief Get flame detection temperature rise threshold (°C)
 * @return Current flame detection threshold
 */
float ptx_oven_get_flame_detect_temp_rise_c(void);

#ifdef __cplusplus
}
#endif

#endif /* PTX_OVEN_CONFIG_H */
