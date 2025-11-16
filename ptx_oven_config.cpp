/**
 * @file ptx_oven_config.cpp
 * @brief Implementation of runtime-configurable oven parameters
 */
#include "ptx_oven_config.h"
#include <stddef.h>

/* Internal configuration state */
static ptx_oven_config_t pti_oven_config = {
    .ignition_duration_ms   = 5000U,  /* 5 seconds igniter ON */
    .periodic_log_ms        = 1000U,  /* log every second */
    .sensor_fault_window_ms = 1000U,  /* fault after 1s out-of-range */
    .auto_resume_delay_ms   = 3000U,  /* resume after 3s valid */
    .vref_min_v             = 4.5f,   /* min vref */
    .vref_max_v             = 5.5f,   /* max vref */
    .temp_target_c          = 180.0f, /* target temperature */
    .temp_delta_c           = 5.0f,   /* hysteresis half-band */
    .max_ignition_attempts  = 3U,     /* 3 ignition retry attempts */
    .purge_time_ms          = 2500U   /* 2.5 seconds purge after failed ignition */
};

const ptx_oven_config_t* ptx_oven_get_config(void) {
    return &pti_oven_config;
}

void ptx_oven_set_config(const ptx_oven_config_t* config) {
    if (config != NULL) {
        pti_oven_config = *config;
    }
}

void ptx_oven_reset_config_to_defaults(void) {
    pti_oven_config.ignition_duration_ms   = 5000U;
    pti_oven_config.periodic_log_ms        = 1000U;
    pti_oven_config.sensor_fault_window_ms = 1000U;
    pti_oven_config.auto_resume_delay_ms   = 3000U;
    pti_oven_config.vref_min_v             = 4.5f;
    pti_oven_config.vref_max_v             = 5.5f;
    pti_oven_config.temp_target_c          = 180.0f;
    pti_oven_config.temp_delta_c           = 2.0f;
    pti_oven_config.max_ignition_attempts  = 3U;
    pti_oven_config.purge_time_ms          = 2500U;
}

/* Individual parameter setters */
void ptx_oven_set_ignition_duration_ms(uint32_t duration_ms) {
    pti_oven_config.ignition_duration_ms = duration_ms;
}

uint32_t ptx_oven_get_ignition_duration_ms(void) {
    return pti_oven_config.ignition_duration_ms;
}

void ptx_oven_set_periodic_log_ms(uint32_t interval_ms) {
    pti_oven_config.periodic_log_ms = interval_ms;
}

uint32_t ptx_oven_get_periodic_log_ms(void) {
    return pti_oven_config.periodic_log_ms;
}

void ptx_oven_set_sensor_fault_window_ms(uint32_t window_ms) {
    pti_oven_config.sensor_fault_window_ms = window_ms;
}

uint32_t ptx_oven_get_sensor_fault_window_ms(void) {
    return pti_oven_config.sensor_fault_window_ms;
}

void ptx_oven_set_auto_resume_delay_ms(uint32_t delay_ms) {
    pti_oven_config.auto_resume_delay_ms = delay_ms;
}

uint32_t ptx_oven_get_auto_resume_delay_ms(void) {
    return pti_oven_config.auto_resume_delay_ms;
}

void ptx_oven_set_vref_range_v(float min_v, float max_v) {
    pti_oven_config.vref_min_v = min_v;
    pti_oven_config.vref_max_v = max_v;
}

float ptx_oven_get_vref_min_v(void) {
    return pti_oven_config.vref_min_v;
}

float ptx_oven_get_vref_max_v(void) {
    return pti_oven_config.vref_max_v;
}

void ptx_oven_set_temp_target_c(float target_c) {
    pti_oven_config.temp_target_c = target_c;
}

float ptx_oven_get_temp_target_c(void) {
    return pti_oven_config.temp_target_c;
}

void ptx_oven_set_temp_delta_c(float delta_c) {
    pti_oven_config.temp_delta_c = delta_c;
}

float ptx_oven_get_temp_delta_c(void) {
    return pti_oven_config.temp_delta_c;
}

void ptx_oven_set_max_ignition_attempts(uint8_t attempts) {
    if (attempts > 0 && attempts <= 5) {
        pti_oven_config.max_ignition_attempts = attempts;
    }
}

uint8_t ptx_oven_get_max_ignition_attempts(void) {
    return pti_oven_config.max_ignition_attempts;
}

void ptx_oven_set_purge_time_ms(uint32_t purge_ms) {
    pti_oven_config.purge_time_ms = purge_ms;
}

uint32_t ptx_oven_get_purge_time_ms(void) {
    return pti_oven_config.purge_time_ms;
}
