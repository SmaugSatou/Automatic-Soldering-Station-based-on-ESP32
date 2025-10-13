/**
 * @file temperature_sensor_hal.h
 * @brief Hardware Abstraction Layer for AD8495 K-Type thermocouple
 * 
 * Provides low-level C interface for reading temperature from AD8495 amplifier.
 * Uses ESP32 ADC to read analog voltage and converts to temperature.
 */

#ifndef TEMPERATURE_SENSOR_HAL_H
#define TEMPERATURE_SENSOR_HAL_H

#include <stdint.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Temperature sensor configuration
 */
typedef struct {
    adc1_channel_t adc_channel;
    adc_bits_width_t adc_width;
    adc_atten_t adc_attenuation;
    uint32_t samples_count;
    double voltage_offset;
    double voltage_scale;
} temperature_sensor_config_t;

/**
 * @brief Temperature sensor handle
 */
typedef struct temperature_sensor_handle_s* temperature_sensor_handle_t;

/**
 * @brief Initialize temperature sensor
 */
temperature_sensor_handle_t temperature_sensor_hal_init(const temperature_sensor_config_t* config);

/**
 * @brief Deinitialize temperature sensor
 */
void temperature_sensor_hal_deinit(temperature_sensor_handle_t handle);

/**
 * @brief Read raw ADC value
 */
uint32_t temperature_sensor_hal_read_raw(temperature_sensor_handle_t handle);

/**
 * @brief Read voltage in millivolts
 */
uint32_t temperature_sensor_hal_read_voltage(temperature_sensor_handle_t handle);

/**
 * @brief Read temperature in Celsius
 */
double temperature_sensor_hal_read_temperature(temperature_sensor_handle_t handle);

/**
 * @brief Calibrate sensor with known temperature
 */
void temperature_sensor_hal_calibrate(temperature_sensor_handle_t handle, double known_temperature);

#ifdef __cplusplus
}
#endif

#endif // TEMPERATURE_SENSOR_HAL_H
