/**
 * @file ptx_sensor_filter.h
 * @brief Sensor reading wrapper with median noise filtering
 * @details Provides filtered temperature sensor readings using median filter
 *          to reject spikes and outliers effectively.
 */
#ifndef PTX_SENSOR_FILTER_H
#define PTX_SENSOR_FILTER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Filtered sensor readings
 */
typedef struct {
    uint16_t vref_mv;           /**< Filtered reference voltage (mV) */
    uint16_t signal_mv;         /**< Filtered signal voltage (mV) */
    bool     valid;             /**< True if filter has enough samples */
} ptx_sensor_reading_t;

/**
 * @brief Initialize median sensor filter
 * @param window_size Number of samples for median calculation (3-10, odd preferred)
 * @note Larger window = better noise rejection but slower response
 */
void ptx_sensor_filter_init(uint8_t window_size);

/**
 * @brief Reset filter state (clear history)
 */
void ptx_sensor_filter_reset(void);

/**
 * @brief Read sensors from hardware and apply median filtering
 * @return Filtered sensor reading
 * @note Call this once per control update cycle
 */
ptx_sensor_reading_t ptx_sensor_filter_read_and_update(void);

/**
 * @brief Get current window size
 * @return Current median filter window size
 */
uint8_t ptx_sensor_filter_get_window_size(void);

#ifdef __cplusplus
}
#endif

#endif /* PTX_SENSOR_FILTER_H */
