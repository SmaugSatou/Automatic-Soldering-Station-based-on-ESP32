/**
 * @file stepper_motor_hal.c
 * @brief Implementation of stepper motor HAL
 *
 * Low-level driver implementation for TMC2208 stepper motor control.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "stepper_motor_hal.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX_STEP_DELAY_US 2500
#define MIN_STEP_DELAY_US 1

static const char *TAG = "STEPPER_HAL";

/**
 * @brief Internal structure for stepper motor handle
 */
struct stepper_motor_handle_s {
    stepper_motor_config_t config;
    bool is_enabled;
    bool is_initialized;
    stepper_direction_t direction;
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

    // Initialize output GPIO pins (step, dir, enable)
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << config->step_pin) |
                        (1ULL << config->dir_pin) |
                        (1ULL << config->enable_pin),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure output GPIO pins");
        free(handle);
        return NULL;
    }

    // Initialize endpoint pin as input with pull-up
    if (config->endpoint_pin != GPIO_NUM_NC) {
        gpio_config_t endpoint_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = (1ULL << config->endpoint_pin),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_ENABLE
        };

        if (gpio_config(&endpoint_conf) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure endpoint GPIO pin");
            free(handle);
            return NULL;
        }
        ESP_LOGI(TAG, "  ENDPOINT: GPIO %d", config->endpoint_pin);
    }


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

void stepper_motor_hal_step(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return;
    }

    if (!handle->is_enabled) {
        ESP_LOGW(TAG, "Motor is not enabled");
        return;
    }

    gpio_set_level(handle->config.step_pin, 1);
    esp_rom_delay_us(200);
    gpio_set_level(handle->config.step_pin, 0);
    esp_rom_delay_us(200);
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

    int32_t l = MIN(steps, MAX_STEP_DELAY_US);
    uint32_t delay = MAX_STEP_DELAY_US;

    for (uint32_t i = 0; i < steps; i++) {
        stepper_motor_hal_step(handle);

        delay = MAX(MIN_STEP_DELAY_US, MAX(l - 2 * (int32_t)i, l + 2 * ((int32_t)i - (int32_t)steps)));  // Simple linear ramp down

        if (i % 100 == 0) {
            ESP_LOGI(TAG, "Progress: %lu/%lu steps", i, steps);
            ESP_LOGI(TAG, "Current delay: %lu ms", delay);
        }

        if (i % 6000 == 5999) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        vTaskDelay(pdMS_TO_TICKS(MAX(delay / 1000, 2)));

        // Log progress every 100 steps
    }

    ESP_LOGI(TAG, "Completed %lu steps", steps);
}

stepper_direction_t stepper_motor_hal_get_direction(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return STEPPER_DIR_CLOCKWISE;
    }

    return handle->direction;
}

bool stepper_motor_hal_endpoint_reached(stepper_motor_handle_t handle) {
    if (handle == NULL || !handle->is_initialized) {
        ESP_LOGW(TAG, "Handle is NULL or not initialized");
        return false;
    }

    if (handle->config.endpoint_pin == GPIO_NUM_NC) {
        ESP_LOGD(TAG, "No endpoint pin configured");
        return false;
    }

    // Assuming active LOW endpoint switch (switch closes to GND)
    bool endpoint_state = !gpio_get_level(handle->config.endpoint_pin);
    if (endpoint_state) {
        ESP_LOGI(TAG, "Endpoint reached!");
    }
    return endpoint_state;
}
