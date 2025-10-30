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

static const char *TAG = "MAIN";

// Global motor instances
static StepperMotor* motor_x = nullptr;
static StepperMotor* motor_y = nullptr;
static StepperMotor* motor_z = nullptr;
static StepperMotor* motor_s = nullptr;

/**
 * @brief Initialize all stepper motors
 */
static void init_motors() {
    ESP_LOGI(TAG, "Initializing stepper motors...");
    
    // X-Axis Motor Configuration
    stepper_motor_config_t config_x = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_X_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_X_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_X_ENABLE_PIN)
    };
    motor_x = new StepperMotor(config_x, CONFIG_STEPS_PER_MM_X);
    if (!motor_x->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize X-axis motor");
        return;
    }
    ESP_LOGI(TAG, "X-axis motor initialized");
    
    // Y-Axis Motor Configuration
    stepper_motor_config_t config_y = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Y_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Y_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Y_ENABLE_PIN)
    };
    motor_y = new StepperMotor(config_y, CONFIG_STEPS_PER_MM_Y);
    if (!motor_y->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize Y-axis motor");
        return;
    }
    ESP_LOGI(TAG, "Y-axis motor initialized");
    
    // Z-Axis Motor Configuration
    stepper_motor_config_t config_z = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Z_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Z_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_Z_ENABLE_PIN)
    };
    motor_z = new StepperMotor(config_z, CONFIG_STEPS_PER_MM_Z);
    if (!motor_z->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize Z-axis motor");
        return;
    }
    ESP_LOGI(TAG, "Z-axis motor initialized");
    
    // Solder Supply Motor Configuration
    stepper_motor_config_t config_s = {
        .step_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_S_STEP_PIN),
        .dir_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_S_DIR_PIN),
        .enable_pin = static_cast<gpio_num_t>(CONFIG_MOTOR_S_ENABLE_PIN)
    };
    motor_s = new StepperMotor(config_s, 80); // Arbitrary steps/mm for solder feed
    if (!motor_s->isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize solder supply motor");
        return;
    }
    ESP_LOGI(TAG, "Solder supply motor initialized");
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
    
    motor_x->setEnable(true);
    motor_x->setTargetPosition(1500);
    motor_y->setEnable(true);
    motor_y->setTargetPosition(1500);
    motor_z->setEnable(true);
    motor_z->setTargetPosition(1500);
    motor_s->setEnable(true);
    motor_s->setTargetPosition(1500);

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20));
        motor_x->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_x->getPosition() - motor_x->getTargetPosition())));
        motor_y->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_y->getPosition() - motor_y->getTargetPosition())));
        motor_z->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_z->getPosition() - motor_z->getTargetPosition())));
        motor_s->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_s->getPosition() - motor_s->getTargetPosition())));

        motor_x->setTargetPosition(-motor_x->getTargetPosition());
        motor_y->setTargetPosition(-motor_y->getTargetPosition());
        motor_z->setTargetPosition(-motor_z->getTargetPosition());
        motor_s->setTargetPosition(-motor_s->getTargetPosition()); 
    }
}
