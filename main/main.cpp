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
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "fsm_controller.h"
#include "stepper_motor_hal.h"
#include "StepperMotor.hpp"

static const char *TAG = "MAIN";

// Global motor instances
static StepperMotor* motor_x = nullptr;
static StepperMotor* motor_y = nullptr;
static StepperMotor* motor_z = nullptr;
static StepperMotor* motor_s = nullptr;

// FSM controller handle
static fsm_controller_handle_t fsm_handle = nullptr;

// Movement patterns for testing - within max limits: X=8000, Y=6000, Z=8000
typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    bool solder;  // Should we simulate soldering at this point?
    uint32_t solder_time_ms;  // How long to apply solder (ms)
} position_t;

// Test pattern: 4 corner soldering points + center point
// Z-axis: 16000 = safe height (up), 18000 = soldering position (down)
static const position_t test_positions[] = {
    {1000, 1000, 16000, false, 0},      // Move to safe height above first point
    {1000, 1000, 18000, true, 2000},     // Lower to solder point 1 (2s)
    {1000, 1000, 16000, false, 0},      // Lift up

    {3000, 1000, 16000, false, 0},      // Move to point 2
    {3000, 1000, 18000, true, 2000},     // Lower to solder point 2 (2s)
    {3000, 1000, 16000, false, 0},      // Lift up

    {3000, 4000, 16000, false, 0},      // Move to point 3
    {3000, 4000, 18000, true, 2000},     // Lower to solder point 3 (2s)
    {3000, 4000, 16000, false, 0},      // Lift up

    {1000, 4000, 16000, false, 0},      // Move to point 4
    {1000, 4000, 18000, true, 2000},     // Lower to solder point 4 (2s)
    {1000, 4000, 16000, false, 0},      // Lift up

    {2000, 2500, 16000, false, 0},      // Move to center point
    {2000, 2500, 18000, true, 3000},     // Lower to solder center (3s)
    {2000, 2500, 16000, false, 0},      // Lift up
};
static const int NUM_POSITIONS = sizeof(test_positions) / sizeof(test_positions[0]);
static int current_position_index = 0;


/**
 * @brief Simulate soldering process
 */
static void simulate_soldering(uint32_t duration_ms) {
    ESP_LOGI(TAG, "  >>> SOLDERING: Starting solder feed...");

    // Enable solder motor
    motor_s->setEnable(true);

    // Feed solder wire slowly (simulate feeding)
    int32_t feed_amount = duration_ms / 10;  // Feed based on time
    motor_s->setTargetPosition(motor_s->getPosition() + feed_amount);

    uint32_t start_time = xTaskGetTickCount();
    uint32_t elapsed = 0;

    while (elapsed < duration_ms) {
        // Move solder motor
        if (motor_s->getPosition() != motor_s->getTargetPosition()) {
            motor_s->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_s->getPosition() - motor_s->getTargetPosition())));
        }

        elapsed = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;

        // Log progress every 500ms
        if (elapsed % 500 < 20) {
            ESP_LOGI(TAG, "  >>> SOLDERING: %.1fs / %.1fs", elapsed / 1000.0f, duration_ms / 1000.0f);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGI(TAG, "  >>> SOLDERING: Complete! Fed %ld steps of solder wire", feed_amount);

    // Disable solder motor
    motor_s->setEnable(false);
}

/**
 * @brief Move motors to target position
 */
static void move_motors_to_position(const position_t* pos) {
    ESP_LOGI(TAG, "Moving to position: X=%ld, Y=%ld, Z=%ld", pos->x, pos->y, pos->z);

    motor_x->setTargetPosition(pos->x);
    motor_y->setTargetPosition(pos->y);
    motor_z->setTargetPosition(pos->z);

    // Move all motors simultaneously
    bool all_reached = false;
    while (!all_reached) {
        bool x_done = (motor_x->getPosition() == motor_x->getTargetPosition());
        bool y_done = (motor_y->getPosition() == motor_y->getTargetPosition());
        bool z_done = (motor_z->getPosition() == motor_z->getTargetPosition());

        if (!x_done) {
            motor_x->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_x->getPosition() - motor_x->getTargetPosition())));
        }
        if (!y_done) {
            motor_y->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_y->getPosition() - motor_y->getTargetPosition())));
        }
        if (!z_done) {
            motor_z->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_z->getPosition() - motor_z->getTargetPosition())));
        }

        all_reached = x_done && y_done && z_done;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI(TAG, "Position reached!");

    // If this position requires soldering, do it
    if (pos->solder) {
        vTaskDelay(pdMS_TO_TICKS(200));  // Brief pause before soldering
        simulate_soldering(pos->solder_time_ms);
        vTaskDelay(pdMS_TO_TICKS(200));  // Brief pause after soldering
    }
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

/**
 * @brief FSM Callback: Enter IDLE state
 */
static bool on_enter_idle(void* user_data) {
    ESP_LOGI(TAG, "[FSM] Entered IDLE state - System ready");
    return true;
}

/**
 * @brief FSM Callback: Enter CALIBRATION state
 */
static bool on_enter_calibration(void* user_data) {
    ESP_LOGI(TAG, "[FSM] Entered CALIBRATION state - Starting calibration...");

    motor_x->calibrate();
    motor_y->calibrate();
    motor_z->calibrate();

    ESP_LOGI(TAG, "[FSM] Calibration complete!");

    // Automatically post success event
    vTaskDelay(pdMS_TO_TICKS(500));
    fsm_controller_post_event(fsm_handle, FSM_EVENT_CALIBRATION_SUCCESS);
    return true;
}

/**
 * @brief FSM Callback: Enter READY state
 */
static bool on_enter_ready(void* user_data) {
    ESP_LOGI(TAG, "[FSM] Entered READY state - Ready to execute movements");
    return true;
}

/**
 * @brief FSM Callback: Enter HEATING state
 */
static bool on_enter_heating(void* user_data) {
    ESP_LOGI(TAG, "[FSM] Entered HEATING state - Preparing for execution...");
    // Since we don't have a real soldering iron, just simulate heating
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "[FSM] Heating complete (simulated)");

    // Post heating success to move to EXECUTING
    fsm_controller_post_event(fsm_handle, FSM_EVENT_HEATING_SUCCESS);
    return true;
}

/**
 * @brief FSM Callback: Enter EXECUTING state
 */
static bool on_enter_executing(void* user_data) {
    ESP_LOGI(TAG, "[FSM] Entered EXECUTING state - Running soldering pattern...");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Starting Soldering Test Pattern     ║");
    ESP_LOGI(TAG, "║   - 4 corner points + 1 center point   ║");
    ESP_LOGI(TAG, "║   - Total: 5 solder joints             ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Enable all motors
    motor_x->setEnable(true);
    motor_y->setEnable(true);
    motor_z->setEnable(true);

    // Count solder points
    int solder_point = 1;

    // Execute movement pattern
    for (int i = 0; i < NUM_POSITIONS; i++) {
        if (test_positions[i].solder) {
            ESP_LOGI(TAG, "═══ Solder Point %d/5 ═══", solder_point++);
        }
        ESP_LOGI(TAG, "Waypoint %d/%d:", i + 1, NUM_POSITIONS);
        move_motors_to_position(&test_positions[i]);
        vTaskDelay(pdMS_TO_TICKS(300));  // Brief pause between movements
    }

    // Return to home
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Returning to home position (0, 0, 0)");
    position_t home = {0, 0, 0, false, 0};
    move_motors_to_position(&home);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Soldering Pattern Complete! ✓        ║");
    ESP_LOGI(TAG, "║   5 joints soldered successfully       ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Post task done event
    vTaskDelay(pdMS_TO_TICKS(500));
    fsm_controller_post_event(fsm_handle, FSM_EVENT_TASK_DONE);
    return true;
}

/**
 * @brief FSM Callback: Enter NORMAL_EXIT state
 */
static bool on_enter_normal_exit(void* user_data) {
    ESP_LOGI(TAG, "[FSM] Entered NORMAL_EXIT state - Task completed successfully!");

    // Disable motors
    motor_x->setEnable(false);
    motor_y->setEnable(false);
    motor_z->setEnable(false);

    return true;
}

/**
 * @brief Initialize FSM controller
 */
static void init_fsm(void) {
    ESP_LOGI(TAG, "Initializing FSM controller...");

    // Configure FSM
    fsm_config_t config = {
        .tick_rate_ms = 100,
        .enable_logging = true,
        .enable_statistics = true,
        .target_temperature = 0.0f,      // Not using temperature
        .temperature_tolerance = 0.0f,
        .heating_timeout_ms = 0,
        .calibration_timeout_ms = 30000
    };

    // Initialize FSM
    fsm_handle = fsm_controller_init(&config);
    if (fsm_handle == nullptr) {
        ESP_LOGE(TAG, "Failed to initialize FSM controller");
        return;
    }

    // Register state callbacks
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_IDLE, on_enter_idle, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_CALIBRATION, on_enter_calibration, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_READY, on_enter_ready, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_HEATING, on_enter_heating, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_EXECUTING, on_enter_executing, nullptr);
    fsm_controller_register_enter_callback(fsm_handle, FSM_STATE_NORMAL_EXIT, on_enter_normal_exit, nullptr);

    // Start FSM
    if (!fsm_controller_start(fsm_handle)) {
        ESP_LOGE(TAG, "Failed to start FSM controller");
        return;
    }

    ESP_LOGI(TAG, "FSM controller initialized successfully");
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

/**
 * @brief Test sequence task - triggers FSM state transitions
 */
static void test_sequence_task(void* pvParameters) {
    // Wait for initialization
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "Starting FSM motor control test sequence");
    ESP_LOGI(TAG, "========================================\n");

    // Step 1: INIT -> IDLE
    ESP_LOGI(TAG, "Step 1: Transitioning to IDLE state");
    fsm_controller_post_event(fsm_handle, FSM_EVENT_INIT_DONE);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Step 2: IDLE -> CALIBRATION
    ESP_LOGI(TAG, "Step 2: Starting calibration");
    fsm_controller_post_event(fsm_handle, FSM_EVENT_REQUEST_CALIBRATION);
    vTaskDelay(pdMS_TO_TICKS(8000));  // Wait for calibration to complete

    // Step 3: CALIBRATION -> READY (automatic via calibration callback)
    ESP_LOGI(TAG, "Step 3: Waiting for READY state");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Step 4: READY -> HEATING (approve task)
    ESP_LOGI(TAG, "Step 4: Approving task to start heating");
    fsm_controller_post_event(fsm_handle, FSM_EVENT_TASK_APPROVED);
    vTaskDelay(pdMS_TO_TICKS(3000));  // Wait for heating

    // Step 5: HEATING -> EXECUTING (automatic via heating callback)
    ESP_LOGI(TAG, "Step 5: Waiting for soldering execution to complete");
    vTaskDelay(pdMS_TO_TICKS(45000));  // Wait for pattern execution (15 movements + 5 solder operations)

    // Step 6: NORMAL_EXIT -> IDLE
    ESP_LOGI(TAG, "Step 6: Returning to IDLE state");
    vTaskDelay(pdMS_TO_TICKS(2000));
    fsm_controller_post_event(fsm_handle, FSM_EVENT_DEINIT_SUCCESS);

    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "Test sequence complete!");
    ESP_LOGI(TAG, "========================================\n");

    // Task done
    vTaskDelete(nullptr);
}

/**
 * @brief Main application entry point
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  FSM Motor Control Test Application");
    ESP_LOGI(TAG, "========================================");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize motors
    init_motors();

    // Initialize FSM controller
    init_fsm();

    // Create FSM processing task
    xTaskCreate(fsm_task, "fsm_task", 4096, nullptr, 5, nullptr);

    // Create test sequence task
    xTaskCreate(test_sequence_task, "test_sequence", 4096, nullptr, 4, nullptr);

    ESP_LOGI(TAG, "System initialized - Starting FSM test...\n");

    // Main loop - just monitor
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
