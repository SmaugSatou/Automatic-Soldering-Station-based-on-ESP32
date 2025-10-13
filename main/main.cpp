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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

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
    
    ESP_LOGI(TAG, "System initialized successfully");
    
    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
