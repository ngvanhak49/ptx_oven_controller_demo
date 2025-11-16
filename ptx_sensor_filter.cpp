/**
 * @file ptx_sensor_filter.cpp
 * @brief Implementation of median sensor filtering
 */
#include "ptx_sensor_filter.h"
#include "api.h"
#include <string.h>

#define PTX_FILTER_MAX_WINDOW 10

/* Filter state */
static struct {
    uint8_t window_size;
    
    /* History buffers for median calculation */
    uint16_t vref_history[PTX_FILTER_MAX_WINDOW];
    uint16_t signal_history[PTX_FILTER_MAX_WINDOW];
    uint8_t history_count;  /* number of valid samples in buffer */
    uint8_t history_index;  /* circular buffer write position */
} pti_filter_state;

/* compute median of buffer (simple bubble sort for small arrays) */
static uint16_t compute_median(const uint16_t* buffer, uint8_t count) {
    uint16_t sorted[PTX_FILTER_MAX_WINDOW];
    memcpy(sorted, buffer, count * sizeof(uint16_t));
    
    /* Bubble sort */
    for (uint8_t i = 0; i < count - 1; i++) {
        for (uint8_t j = 0; j < count - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                uint16_t temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    /* Return middle value */
    if (count % 2 == 0) {
        return (sorted[count / 2 - 1] + sorted[count / 2]) / 2;
    } else {
        return sorted[count / 2];
    }
}

void ptx_sensor_filter_init(uint8_t window_size) {
    pti_filter_state.window_size = (window_size > PTX_FILTER_MAX_WINDOW) ? PTX_FILTER_MAX_WINDOW : window_size;
    if (pti_filter_state.window_size < 3) pti_filter_state.window_size = 3; // minimum 3 for median
    
    ptx_sensor_filter_reset();
}

void ptx_sensor_filter_reset(void) {
    pti_filter_state.history_count = 0;
    pti_filter_state.history_index = 0;
    memset(pti_filter_state.vref_history, 0, sizeof(pti_filter_state.vref_history));
    memset(pti_filter_state.signal_history, 0, sizeof(pti_filter_state.signal_history));
}

ptx_sensor_reading_t ptx_sensor_filter_update(uint16_t raw_vref_mv, 
                                                uint16_t raw_signal_mv) {
    ptx_sensor_reading_t result = {0};
    
    /* Add to circular buffer */
    pti_filter_state.vref_history[pti_filter_state.history_index] = raw_vref_mv;
    pti_filter_state.signal_history[pti_filter_state.history_index] = raw_signal_mv;
    pti_filter_state.history_index = (pti_filter_state.history_index + 1) % pti_filter_state.window_size;
    
    if (pti_filter_state.history_count < pti_filter_state.window_size) {
        pti_filter_state.history_count++;
    }
    
    /* Need full window before filter is valid */
    if (pti_filter_state.history_count >= pti_filter_state.window_size) {
        result.vref_mv = compute_median(pti_filter_state.vref_history, pti_filter_state.window_size);
        result.signal_mv = compute_median(pti_filter_state.signal_history, pti_filter_state.window_size);
        result.valid = true;
    } else {
        /* Not enough samples yet; return raw */
        result.vref_mv = raw_vref_mv;
        result.signal_mv = raw_signal_mv;
        result.valid = false;
    }
    
    return result;
}

ptx_sensor_reading_t ptx_sensor_filter_read_and_update(void) {
    /* Read raw sensor values from hardware */
    uint16_t raw_vref_mv   = read_voltage(TEMPERATURE_SENSOR_REFERENCE);
    uint16_t raw_signal_mv = read_voltage(TEMPERATURE_SENSOR);
    
    /* Apply median filter */
    return ptx_sensor_filter_update(raw_vref_mv, raw_signal_mv);
}

uint8_t ptx_sensor_filter_get_window_size(void) {
    return pti_filter_state.window_size;
}
