/**
 * @file stepper_motor_hal.c
 * @brief Implementation of stepper motor HAL
 * 
 * Low-level driver implementation for DRV8825 stepper motor control.
 */

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_log.h"

#include "stepper_motor_hal.h"

/**
 * @brief Internal structure for stepper motor handle
 */
struct stepper_motor_handle_s {
    stepper_motor_config_t config;
    bool is_enabled;
    bool is_initialized;
    stepper_direction_t direction;
    stepper_microstep_mode_t microstep_mode;
    uint32_t step_time_us;  // Time between steps in microseconds
};

stepper_motor_handle_t stepper_motor_hal_init(const stepper_motor_config_t* config)
{
    // Check if given pointer valid
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return NULL;
    }

    // Allocate memory for handle and check for errors
    stepper_motor_handle_t handle = malloc(sizeof(struct stepper_motor_handle_s));
    if (handle == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for stepper motor handle");
        return NULL;
    }

    handle->config = *config;
    handle->is_enabled = false;
    handle->is_initialized = true;
    handle->direction = STEPPER_DIR_CLOCKWISE;
    handle->microstep_mode = STEPPER_MICROSTEP_1_4;
    handle->step_time_us = 1000; // Default 1ms between steps

    // Initialize GPIO pins
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << config->step_pin) |
                        (1ULL << config->dir_pin) |
                        (1ULL << config->enable_pin) |
                        (1ULL << config->mode0_pin) |
                        (1ULL << config->mode1_pin),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };


    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        free(handle);
        return NULL;
    }

    // Set initial states
    gpio_set_level(config->enable_pin, 1);
    // Set default direction to clockwise
    gpio_set_level(config->dir_pin, 0);
    // Set default microstepping mode to the biggest step
    gpio_set_level(config->mode0_pin, 0);
    gpio_set_level(config->mode1_pin, 0);

    return handle;
}

void stepper_motor_hal_deinit(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    // Disable motor
    stepper_motor_hal_set_enable(handle, false);

    // Free allocated memory
    free(handle);
}

void stepper_motor_hal_set_enable(stepper_motor_handle_t handle, bool enable) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    gpio_set_level(handle->config.enable_pin, enable ? STEPPER_ENABLE : STEPPER_DISABLE);
    handle->is_enabled = enable;
}

void stepper_motor_hal_set_direction(stepper_motor_handle_t handle, stepper_direction_t direction) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    if (direction != STEPPER_DIR_CLOCKWISE && direction != STEPPER_DIR_COUNTERCLOCKWISE) {
        ESP_LOGW(TAG, "Invalid direction");
        return;
    }

    gpio_set_level(handle->config.dir_pin, direction == STEPPER_DIR_CLOCKWISE ? 0 : 1);
    handle->direction = direction;
}

void stepper_motor_hal_set_microstep_mode(stepper_motor_handle_t handle, stepper_microstep_t microstep) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    switch (microstep) {
        case STEPPER_MICROSTEP_1_4:
            gpio_set_level(handle->config.mode0_pin, 0);
            gpio_set_level(handle->config.mode1_pin, 0);
            break;
        case STEPPER_MICROSTEP_1_8:
            gpio_set_level(handle->config.mode0_pin, 1);
            gpio_set_level(handle->config.mode1_pin, 0);
            break;
        case STEPPER_MICROSTEP_1_16:
            gpio_set_level(handle->config.mode0_pin, 0);
            gpio_set_level(handle->config.mode1_pin, 1);
            break;
        case STEPPER_MICROSTEP_1_32:
            gpio_set_level(handle->config.mode0_pin, 1);
            gpio_set_level(handle->config.mode1_pin, 1);
            break;
        default:
            ESP_LOGW(TAG, "Invalid microstepping mode");
            return;
    }
    handle->config.microstep_mode = microstep;
}

void stepper_motor_hal_step(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    if (!handle->is_enabled) {
        ESP_LOGW(TAG, "Motor is not enabled");
        return;
    }

    // Generate a step pulse
    gpio_set_level(handle->config.step_pin, 1);
    ets_delay_us(handle->step_time_us); 
    gpio_set_level(handle->config.step_pin, 0);
}

void stepper_motor_hal_step_multiple(stepper_motor_handle_t handle, uint32_t steps) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    if (!handle->is_enabled) {
        ESP_LOGW(TAG, "Motor is not enabled");
        return;
    }

    for (uint32_t i = 0; i < steps; i++) {
        stepper_motor_hal_step(handle);
        ets_delay_us(handle->step_time_us);
    }
}

void stepper_motor_hal_set_step_time(stepper_motor_handle_t handle, uint32_t step_time_us) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    if (step_time_us < 100) {
        ESP_LOGW(TAG, "Step time too short, setting to minimum 100us");
        step_time_us = 100;
    }

    handle->step_time_us = step_time_us;
    ESP_LOGI(TAG, "Step time set to %lu us (%.1f Hz)", step_time_us, 1000000.0f / step_time_us);
}

uint32_t stepper_motor_hal_get_step_time(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return 0;
    }

    return handle->step_time_us;
}