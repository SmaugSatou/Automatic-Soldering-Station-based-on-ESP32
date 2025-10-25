/**
 * @file stepper_motor_hal.c
 * @brief Implementation of stepper motor HAL
 * 
 * Low-level driver implementation for TMC2208 stepper motor control.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "stepper_motor_hal.h"

static const char *TAG = "STEPPER_HAL";

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

    ESP_LOGI(TAG, "Initializing stepper motor with pins:");
    ESP_LOGI(TAG, "  STEP:   GPIO %d", config->step_pin);
    ESP_LOGI(TAG, "  DIR:    GPIO %d", config->dir_pin);
    ESP_LOGI(TAG, "  ENABLE: GPIO %d", config->enable_pin);
    ESP_LOGI(TAG, "  MS1:    GPIO %d", config->mode0_pin);
    ESP_LOGI(TAG, "  MS2:    GPIO %d", config->mode1_pin);

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

    ESP_LOGI(TAG, "GPIO pins configured successfully");

    // Set initial states
    // TMC2208: ENABLE pin is active LOW (0 = enabled, 1 = disabled)
    gpio_set_level(config->enable_pin, 1);  // Start disabled
    ESP_LOGI(TAG, "ENABLE pin set HIGH (motor disabled)");
    
    // Set default direction to clockwise
    gpio_set_level(config->dir_pin, 0);
    ESP_LOGI(TAG, "DIR pin set LOW (clockwise)");
    
    // TMC2208: Set default microstepping mode
    // MS1=0, MS2=0 for 1/8 step (default after power-on)
    gpio_set_level(config->mode0_pin, 0);
    gpio_set_level(config->mode1_pin, 0);
    ESP_LOGI(TAG, "MS pins set for 1/8 microstepping");

    ESP_LOGI(TAG, "Stepper motor initialization complete");
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

    // TMC2208: ENABLE is active LOW (0 = enabled, 1 = disabled)
    gpio_set_level(handle->config.enable_pin, enable ? 0 : 1);
    handle->is_enabled = enable;
    ESP_LOGI(TAG, "Motor %s", enable ? "ENABLED" : "DISABLED");
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
    ESP_LOGI(TAG, "Direction set to %s (DIR pin = %d)", 
             direction == STEPPER_DIR_CLOCKWISE ? "CLOCKWISE" : "COUNTERCLOCKWISE",
             direction == STEPPER_DIR_CLOCKWISE ? 0 : 1);
}

void stepper_motor_hal_set_microstep_mode(stepper_motor_handle_t handle, stepper_microstep_mode_t microstep) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    // TMC2208 Microstepping Configuration (MS1, MS2):
    // MS2 MS1 | Microsteps
    // 0   0   | 1/8 step (default)
    // 0   1   | 1/2 step  
    // 1   0   | 1/4 step
    // 1   1   | 1/16 step
    // Note: 1/32 requires StealthChop mode via UART, using 1/16 instead
    
    switch (microstep) {
        case STEPPER_MICROSTEP_1_4:
            gpio_set_level(handle->config.mode0_pin, 0);  // MS1 = 0
            gpio_set_level(handle->config.mode1_pin, 1);  // MS2 = 1
            ESP_LOGI(TAG, "Microstep mode set to 1/4");
            break;
        case STEPPER_MICROSTEP_1_8:
            gpio_set_level(handle->config.mode0_pin, 0);  // MS1 = 0
            gpio_set_level(handle->config.mode1_pin, 0);  // MS2 = 0
            ESP_LOGI(TAG, "Microstep mode set to 1/8");
            break;
        case STEPPER_MICROSTEP_1_16:
            gpio_set_level(handle->config.mode0_pin, 1);  // MS1 = 1
            gpio_set_level(handle->config.mode1_pin, 1);  // MS2 = 1
            ESP_LOGI(TAG, "Microstep mode set to 1/16");
            break;
        case STEPPER_MICROSTEP_1_32:
            // TMC2208 doesn't support 1/32 via MS pins, use 1/16 instead
            gpio_set_level(handle->config.mode0_pin, 1);  // MS1 = 1
            gpio_set_level(handle->config.mode1_pin, 1);  // MS2 = 1
            ESP_LOGW(TAG, "1/32 microstep not supported on TMC2208, using 1/16 instead");
            break;
        default:
            ESP_LOGW(TAG, "Invalid microstepping mode");
            return;
    }
    handle->microstep_mode = microstep;
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

    // Generate a step pulse (TMC2208 requires minimum 100ns pulse width, using 5us to be safe)
    gpio_set_level(handle->config.step_pin, 1);
    esp_rom_delay_us(5);  // 5 microsecond pulse
    gpio_set_level(handle->config.step_pin, 0);
    esp_rom_delay_us(5);  // 5 microsecond low time before next pulse
}

void stepper_motor_hal_step_multiple(stepper_motor_handle_t handle, uint32_t steps) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    if (!handle->is_enabled) {
        ESP_LOGE(TAG, "Motor is not enabled! Cannot step.");
        return;
    }

    ESP_LOGI(TAG, "Stepping %lu times with %lu us delay between steps", steps, handle->step_time_us);
    
    for (uint32_t i = 0; i < steps; i++) {
        stepper_motor_hal_step(handle);
        esp_rom_delay_us(handle->step_time_us);
        
        // Log progress every 100 steps
        if ((i + 1) % 100 == 0) {
            ESP_LOGI(TAG, "Progress: %lu/%lu steps", i + 1, steps);
        }
    }
    
    ESP_LOGI(TAG, "Completed %lu steps", steps);
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

stepper_direction_t stepper_motor_hal_get_direction(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return STEPPER_DIR_CLOCKWISE;
    }

    return handle->direction;
}

stepper_microstep_mode_t stepper_motor_hal_get_microstep_mode(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return STEPPER_MICROSTEP_1_4;
    }

    return handle->microstep_mode;
}