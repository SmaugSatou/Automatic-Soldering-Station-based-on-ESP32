/**
 * @file main.cpp
 * @brief Main application entry point for automatic soldering station
 * 
 * Initializes all hardware components, starts WiFi AP and web server,
 * manages G-Code execution, and coordinates system operation.
 * 
 * @author UCU Automatic Soldering Station Team
 * @date 2025
 */

#include <stdio.h>
#include <cmath>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "wifi_manager.h"
#include "web_server.h"
#include "filesystem.h"
#include "stepper_motor_hal.h"
#include "StepperMotor.hpp"
#include "soldering_iron_hal.h"
#include "SolderingIron.hpp"
#include "temperature_sensor_hal.h"
#include "TemperatureSensor.hpp"
#include "motion_controller.h"
#include "gcode_parser.h"
#include "gcode_executor.h"
#include "display.h"
#include "esp_random.h"

static const char *TAG = "MAIN";

// Global motor instances
static StepperMotor* motor_x = nullptr;
static StepperMotor* motor_y = nullptr;
static StepperMotor* motor_z = nullptr;
static StepperMotor* motor_s = nullptr;


static int32_t get_random_position(int32_t min, int32_t max) {
    uint32_t random = esp_random();  // Get 32-bit random number
    return min + (random % (max - min + 1));
}

/**
 * @brief Initialize all stepper motors
 */
static void init_motors() {
    ESP_LOGI(TAG, "Initializing stepper motors...");
    
    // X-Axis Motor Configuration
    stepper_motor_config_t config_x = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_X_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_X_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_X_ENABLE_PIN),
        .endpoint_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_X_MIN_ENDPOINT_PIN)  // Changed from ENDPOINT to MIN_ENDPOINT
    };
    motor_x = new StepperMotor(config_x, CONFIG_STEPS_PER_MM_X, STEPPER_DIR_COUNTERCLOCKWISE);
    if (!motor_x->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize X-axis motor");
        return;
    }
    ESP_LOGI(TAG, "X-axis motor initialized");
    
    // Y-Axis Motor Configuration
    stepper_motor_config_t config_y = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Y_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Y_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Y_ENABLE_PIN),
        .endpoint_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Y_MIN_ENDPOINT_PIN)  // Changed from ENDPOINT to MIN_ENDPOINT
    };
    motor_y = new StepperMotor(config_y, CONFIG_STEPS_PER_MM_Y, STEPPER_DIR_CLOCKWISE);
    if (!motor_y->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize Y-axis motor");
        return;
    }
    ESP_LOGI(TAG, "Y-axis motor initialized");
    
    // Z-Axis Motor Configuration
    stepper_motor_config_t config_z = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Z_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Z_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Z_ENABLE_PIN),
        .endpoint_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Z_MIN_ENDPOINT_PIN)  // Changed from ENDPOINT to MIN_ENDPOINT
    };
    motor_z = new StepperMotor(config_z, CONFIG_STEPS_PER_MM_Z, STEPPER_DIR_CLOCKWISE);
    if (!motor_z->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize Z-axis motor");
        return;
    }
    ESP_LOGI(TAG, "Z-axis motor initialized");
    
    // Solder Supply Motor Configuration
    stepper_motor_config_t config_s = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_S_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_S_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_S_ENABLE_PIN),
        .endpoint_pin = GPIO_NUM_NC  // No endpoint switch for solder supply
    };
    motor_s = new StepperMotor(config_s, 80); // Arbitrary steps/mm for solder feed
    if (!motor_s->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize solder supply motor");
        return;
    }
    ESP_LOGI(TAG, "Solder supply motor initialized");
}

void calibrate_motors() {
    ESP_LOGI(TAG, "Calibrating motors...");
    
    
    
    ESP_LOGI(TAG, "Motors calibrated");
}

/**
 * @brief Main application entry point
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Automatic Soldering Station starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize motors
    init_motors();
    
    // Create motor test task
    // xTaskCreate(motor_test_task, "motor_test", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "System initialized successfully");
    ESP_LOGI(TAG, "Calibrating motors...");

    motor_x->calibrate();
    motor_y->calibrate();
    motor_z->calibrate();

    ESP_LOGI(TAG, "Calibration complete");
    
    motor_x->setEnable(true);
    motor_x->setTargetPosition(2000);
    motor_y->setEnable(true);
    motor_y->setTargetPosition(2000);
    motor_z->setEnable(true);
    motor_z->setTargetPosition(2000);
    motor_s->setEnable(true);
    motor_s->setTargetPosition(2000);

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20));
        motor_x->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_x->getPosition() - motor_x->getTargetPosition())));
        vTaskDelay(pdMS_TO_TICKS(20));
        motor_y->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_y->getPosition() - motor_y->getTargetPosition())));
        vTaskDelay(pdMS_TO_TICKS(20));
        motor_z->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_z->getPosition() - motor_z->getTargetPosition())));
        // vTaskDelay(pdMS_TO_TICKS(20));
        // motor_s->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_s->getPosition() - motor_s->getTargetPosition())));

        motor_x->setTargetPosition(get_random_position(10, 6000));
        motor_y->setTargetPosition(get_random_position(10, 5000));
        motor_z->setTargetPosition(get_random_position(10, 8000));
        // motor_s->setTargetPosition(get_random_position(10, 5000));
    }
}
