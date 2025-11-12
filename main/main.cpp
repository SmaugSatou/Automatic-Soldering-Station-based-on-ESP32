/**
 * @file main.cpp
 * @brief Simple FSM-based motor control application
 *
 * Uses FSM controller to manage motor movements through different states.
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
#include "esp_timer.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "fsm_controller.h"
#include "stepper_motor_hal.h"
#include "StepperMotor.hpp"
#include "execution_fsm.h"

static const char *TAG = "MAIN";

// Global motor instances (exported for execution_fsm)
StepperMotor* motor_x = nullptr;
StepperMotor* motor_y = nullptr;
StepperMotor* motor_z = nullptr;
StepperMotor* motor_s = nullptr;

// FSM controller handle
static fsm_controller_handle_t fsm_handle = nullptr;

// Test pattern: 23 solder points (4 corners + center) - positions in mm, will be converted to steps
static constexpr int NUM_SOLDER_POINTS = 23;
static solder_point_t solder_points[NUM_SOLDER_POINTS];

static void init_solder_points() {
    // Define positions in mm
    int32_t x_mm[NUM_SOLDER_POINTS] = {0, 
        250, 0, 1, 2, 4, 8, 10, 20, 40, 80, 160,
        160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160};      // X positions in mm
    int32_t y_mm[NUM_SOLDER_POINTS] = {0, 
        220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220,
        220, 0, 1, 2, 4, 8, 10, 20, 40, 80, 160};     // Y positions in mm
    int32_t z_mm[NUM_SOLDER_POINTS] = {0};             // Z positions in mm
    
    // Convert to steps using motor-specific conversion
    for (int i = 0; i < NUM_SOLDER_POINTS; i++) {
        solder_points[i].x = motor_x->mm_to_microsteps(x_mm[i]);
        solder_points[i].y = motor_y->mm_to_microsteps(y_mm[i]);
        solder_points[i].z = motor_z->mm_to_microsteps(z_mm[i]);
        solder_points[i].solder = true;
        solder_points[i].solder_time_ms = 2000;
    }
}

// Execution sub-FSM instance (initialized in on_enter_executing)
static execution_sub_fsm_t exec_sub_fsm;

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
    motor_x = new StepperMotor(config_x, CONFIG_MOTOR_X_MICROSTEPS_IN_MM, STEPPER_DIR_COUNTERCLOCKWISE);
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
    motor_y = new StepperMotor(config_y, CONFIG_MOTOR_Y_MICROSTEPS_IN_MM, STEPPER_DIR_CLOCKWISE);
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
    motor_z = new StepperMotor(config_z, CONFIG_MOTOR_Z_MICROSTEPS_IN_MM, STEPPER_DIR_CLOCKWISE);
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

static bool on_enter_idle(void* user_data) {
    ESP_LOGI(TAG, "FSM: IDLE - System ready");
    return true;
}

static bool on_enter_calibration(void* user_data) {
    ESP_LOGI(TAG, "FSM: CALIBRATION");
    return true;
}

static bool on_execute_calibration(void* user_data) {
    fsm_execution_context_t* ctx = fsm_controller_get_execution_context(fsm_handle);
    if (!ctx) return false;

    if (ctx->iteration_count == 0) {
        ESP_LOGI(TAG, "Calibrating X-axis");
        motor_x->calibrate();
        ctx->iteration_count = 1;
    } else if (ctx->iteration_count == 1) {
        ESP_LOGI(TAG, "Calibrating Y-axis");
        motor_y->calibrate();
        ctx->iteration_count = 2;
    } else if (ctx->iteration_count == 2) {
        ESP_LOGI(TAG, "Calibrating Z-axis");
        motor_z->calibrate();
        ctx->iteration_count = 3;
    } else if (ctx->iteration_count == 3 && !ctx->operation_complete) {
        uint32_t time_since_start = (esp_timer_get_time() / 1000) - ctx->start_time_ms;
        if (time_since_start > 500) {
            ESP_LOGI(TAG, "Calibration complete");
            ctx->operation_complete = true;
            fsm_controller_post_event(fsm_handle, FSM_EVENT_CALIBRATION_SUCCESS);
        }
    }

    return true;
}

static bool on_enter_ready(void* user_data) {
    ESP_LOGI(TAG, "FSM: READY - Task approved, awaiting start");
    return true;
}

static bool on_enter_heating(void* user_data) {
    ESP_LOGI(TAG, "FSM: HEATING (simulated)");
    return true;
}

static bool on_execute_heating(void* user_data) {
    fsm_execution_context_t* ctx = fsm_controller_get_execution_context(fsm_handle);
    if (!ctx) return false;

    uint32_t time_heating = (esp_timer_get_time() / 1000) - ctx->start_time_ms;

    if (time_heating >= 2000 && !ctx->operation_complete) {
        ESP_LOGI(TAG, "Heating complete");
        ctx->operation_complete = true;
        fsm_controller_post_event(fsm_handle, FSM_EVENT_HEATING_SUCCESS);
    }

    return true;
}

static bool on_enter_executing(void* user_data) {
    ESP_LOGI(TAG, "FSM: EXECUTING - %d solder points", NUM_SOLDER_POINTS);

    motor_x->setEnable(true);
    motor_y->setEnable(true);
    motor_z->setEnable(true);

    execution_config_t exec_config = {
        .safe_z_height = motor_z->mm_to_microsteps(160),       // 160mm in steps
        .soldering_z_height = motor_z->mm_to_microsteps(180),  // 180mm in steps
        .home_x = 0,
        .home_y = 0,
        .home_z = 0
    };

    exec_sub_fsm_init(&exec_sub_fsm, &exec_config);
    return true;
}

static bool on_execute_executing(void* user_data) {
    exec_sub_fsm_process(&exec_sub_fsm, solder_points, NUM_SOLDER_POINTS);

    if (exec_sub_fsm_get_state(&exec_sub_fsm) == EXEC_STATE_COMPLETE) {
        ESP_LOGI(TAG, "Execution complete: %d/%d points",
                 exec_sub_fsm_get_completed_count(&exec_sub_fsm), NUM_SOLDER_POINTS);
        fsm_controller_post_event(fsm_handle, FSM_EVENT_TASK_DONE);
    }

    return true;
}

static bool on_enter_normal_exit(void* user_data) {
    ESP_LOGI(TAG, "FSM: NORMAL_EXIT - Cleanup and cooldown");

    // Motors already at home, disable them
    motor_x->setEnable(false);
    motor_y->setEnable(false);
    motor_z->setEnable(false);
    motor_s->setEnable(false);

    fsm_execution_context_t* ctx = fsm_controller_get_execution_context(fsm_handle);
    if (ctx) {
        fsm_execution_context_init(ctx);
        ctx->operation_complete = false;
    }

    return true;
}

static bool on_execute_normal_exit(void* user_data) {
    fsm_execution_context_t* ctx = fsm_controller_get_execution_context(fsm_handle);
    if (!ctx) return false;

    uint32_t time_cooldown = (esp_timer_get_time() / 1000) - ctx->start_time_ms;
    const uint32_t COOLDOWN_TIME = 5000; // 5s simulated cooldown

    // In real system: check temperature sensor instead
    // if (get_iron_temperature() <= safe_temp) { ... }

    if (time_cooldown >= COOLDOWN_TIME && !ctx->operation_complete) {
        ESP_LOGI(TAG, "Cooldown complete - System safe");
        ctx->operation_complete = true;
        fsm_controller_post_event(fsm_handle, FSM_EVENT_COOLDOWN_COMPLETE);
    }

    return true;
}

static void init_fsm(void) {
    init_solder_points();
    
    fsm_config_t config = {
        .tick_rate_ms = 100,
        .enable_logging = true,
        .enable_statistics = true,
        .target_temperature = 350.0f,
        .temperature_tolerance = 5.0f,
        .heating_timeout_ms = 60000,
        .calibration_timeout_ms = 30000,
        .safe_temperature = 50.0f,
        .cooldown_timeout_ms = 120000
    };

    fsm_handle = fsm_controller_init(&config);
    if (!fsm_handle) {
        ESP_LOGE(TAG, "FSM init failed");
        return;
    }

    // Register callbacks
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_IDLE, on_enter_idle, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_CALIBRATION, on_enter_calibration, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_READY, on_enter_ready, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_HEATING, on_enter_heating, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_EXECUTING, on_enter_executing, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_NORMAL_EXIT, on_enter_normal_exit, nullptr);

    fsm_controller_register_execute_callback(fsm_handle, FSM_STATE_CALIBRATION, on_execute_calibration, nullptr);
    fsm_controller_register_execute_callback(fsm_handle, FSM_STATE_HEATING, on_execute_heating, nullptr);
    fsm_controller_register_execute_callback(fsm_handle, FSM_STATE_EXECUTING, on_execute_executing, nullptr);
    fsm_controller_register_execute_callback(fsm_handle, FSM_STATE_NORMAL_EXIT, on_execute_normal_exit, nullptr);

    if (!fsm_controller_start(fsm_handle)) {
        ESP_LOGE(TAG, "FSM start failed");
        return;
    }

    ESP_LOGI(TAG, "FSM initialized");
}

/**
 * @brief FSM processing task
 */
static void fsm_task(void* pvParameters) {
    while (1) {
        fsm_controller_process(fsm_handle);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void test_sequence_task(void* pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "=== Test Sequence Start ===");

    fsm_controller_post_event(fsm_handle, FSM_EVENT_INIT_DONE);
    vTaskDelay(pdMS_TO_TICKS(1000));

    fsm_controller_post_event(fsm_handle, FSM_EVENT_REQUEST_CALIBRATION);
    vTaskDelay(pdMS_TO_TICKS(8000));

    vTaskDelay(pdMS_TO_TICKS(2000));

    fsm_controller_post_event(fsm_handle, FSM_EVENT_TASK_APPROVED);
    vTaskDelay(pdMS_TO_TICKS(3000));

    vTaskDelay(pdMS_TO_TICKS(45000));

    vTaskDelay(pdMS_TO_TICKS(8000));

    ESP_LOGI(TAG, "=== Test Sequence Complete ===");

    vTaskDelete(nullptr);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== Automatic Soldering Station ===");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_motors();
    init_fsm();

    xTaskCreate(fsm_task, "fsm_task", 4096, nullptr, 5, nullptr);
    xTaskCreate(test_sequence_task, "test_sequence", 4096, nullptr, 4, nullptr);

    ESP_LOGI(TAG, "System initialized");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
