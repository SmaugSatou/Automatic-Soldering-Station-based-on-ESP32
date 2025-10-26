/**
 * @file StepperMotor.cpp
 * @brief Implementation of C++ stepper motor wrapper
 * 
 * Provides object-oriented interface to stepper motor HAL.
 */

#include "StepperMotor.hpp"
#include "esp_log.h"
#include <stdexcept>

static const char *TAG = "StepperMotor";

// Constructor
StepperMotor::StepperMotor(const stepper_motor_config_t& config, uint32_t steps_per_mm) :
    position_(0), 
    handle_(stepper_motor_hal_init(&config)), 
    target_position_(0),
    steps_per_mm(steps_per_mm)
{
    if (handle_ == nullptr) {
        ESP_LOGE(TAG, "Failed to initialize stepper motor");
        // Cannot throw in ESP-IDF (exceptions disabled)
        // User should check isInitialized() after construction
    } else {
        ESP_LOGI(TAG, "StepperMotor initialized with %lu steps/mm", steps_per_mm);
    }
}

// Destructor
StepperMotor::~StepperMotor() {
    if (handle_ != nullptr) {
        stepper_motor_hal_deinit(handle_);
        handle_ = nullptr;
    }
}

// Move constructor
StepperMotor::StepperMotor(StepperMotor&& other) noexcept : 
    position_(other.position_), 
    handle_(other.handle_), 
    target_position_(other.target_position_),
    steps_per_mm(other.steps_per_mm) 
{
    other.handle_ = nullptr;
    other.position_ = 0;
    other.target_position_ = 0;
}

// Move assignment operator
StepperMotor& StepperMotor::operator=(StepperMotor&& other) noexcept {
    if (this != &other) {
        // Clean up existing handle
        if (handle_ != nullptr) {
            stepper_motor_hal_deinit(handle_);
        }
        
        // Move data
        position_ = other.position_;
        handle_ = other.handle_;
        target_position_ = other.target_position_;
        steps_per_mm = other.steps_per_mm;
        
        // Reset other
        other.handle_ = nullptr;
        other.position_ = 0;
        other.target_position_ = 0;
    }
    return *this;
}

void StepperMotor::setEnable(bool enable) {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    stepper_motor_hal_set_enable(handle_, enable);
}

void StepperMotor::setDirection(stepper_direction_t direction) {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    stepper_motor_hal_set_direction(handle_, direction);
}

void StepperMotor::setMicrostepMode(stepper_microstep_mode_t mode) {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    stepper_motor_hal_set_microstep_mode(handle_, mode);
}

void StepperMotor::step() {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    stepper_motor_hal_step(handle_);
    
    // Update internal position tracking in 1/256 steps units
    // Position delta depends on current microstepping mode:
    // STEPPER_MICROSTEP_1_4  -> 1 step = 64/256 (full step / 4)
    // STEPPER_MICROSTEP_1_8  -> 1 step = 32/256 (full step / 8)
    // STEPPER_MICROSTEP_1_16 -> 1 step = 16/256 (full step / 16)
    // STEPPER_MICROSTEP_1_32 -> 1 step = 8/256  (full step / 32)
    
    int32_t position_delta = 0;
    stepper_microstep_mode_t mode = stepper_motor_hal_get_microstep_mode(handle_);
    
    switch (mode) {
        case STEPPER_MICROSTEP_1_4:
            position_delta = 64;
            break;
        case STEPPER_MICROSTEP_1_8:
            position_delta = 32;
            break;
        case STEPPER_MICROSTEP_1_16:
            position_delta = 16;
            break;
        case STEPPER_MICROSTEP_1_32:
            position_delta = 8;
            break;
    }
    
    // Apply direction
    if (stepper_motor_hal_get_direction(handle_) == STEPPER_DIR_CLOCKWISE) {
        position_ += position_delta;
    } else {
        position_ -= position_delta;
    }
}

void StepperMotor::stepMultiple(uint32_t steps) {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    stepper_motor_hal_step_multiple(handle_, steps);
    
    // Update internal position tracking in 1/256 steps units
    int32_t position_delta = 0;
    stepper_microstep_mode_t mode = stepper_motor_hal_get_microstep_mode(handle_);
    
    switch (mode) {
        case STEPPER_MICROSTEP_1_4:
            position_delta = 64;
            break;
        case STEPPER_MICROSTEP_1_8:
            position_delta = 32;
            break;
        case STEPPER_MICROSTEP_1_16:
            position_delta = 16;
            break;
        case STEPPER_MICROSTEP_1_32:
            position_delta = 8;
            break;
    }
    
    // Apply direction and number of steps
    if (stepper_motor_hal_get_direction(handle_) == STEPPER_DIR_CLOCKWISE) {
        position_ += position_delta * steps;
    } else {
        position_ -= position_delta * steps;
    }
}

void StepperMotor::setTargetPosition(int32_t position) {
    target_position_ = position;
}

void StepperMotor::stepToTarget() {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    
    int32_t current_pos = getPosition();
    
    if (current_pos < target_position_) {
        // Move forward
        stepper_motor_hal_set_direction(handle_, STEPPER_DIR_CLOCKWISE);
        step();  // Use step() method which handles position tracking
    } else if (current_pos > target_position_) {
        // Move backward
        stepper_motor_hal_set_direction(handle_, STEPPER_DIR_COUNTERCLOCKWISE);
        step();  // Use step() method which handles position tracking
    }
    // If current_pos == target_position_, do nothing
}

void StepperMotor::stepMultipleToTarget(uint32_t max_steps) {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    
    int32_t current_pos = getPosition();
    int32_t remaining_steps = target_position_ - current_pos;
    
    if (remaining_steps == 0) {
        return; // Already at target
    }
    
    // Determine direction
    if (remaining_steps > 0) {
        stepper_motor_hal_set_direction(handle_, STEPPER_DIR_CLOCKWISE);
    } else {
        stepper_motor_hal_set_direction(handle_, STEPPER_DIR_COUNTERCLOCKWISE);
        remaining_steps = -remaining_steps; // Make positive
    }
    
    // Limit steps to max_steps
    uint32_t steps_to_execute = (remaining_steps > (int32_t)max_steps) ? max_steps : (uint32_t)remaining_steps;
    
    stepMultiple(steps_to_execute);  // Use stepMultiple() which handles position tracking
}

int32_t StepperMotor::getPosition() const {
    return position_;
}

int32_t StepperMotor::getTargetPosition() const {
    return target_position_;
}

void StepperMotor::resetPosition() {
    if (handle_ == nullptr) {
        ESP_LOGW(TAG, "Motor not initialized");
        return;
    }
    position_ = 0;
    target_position_ = 0;
    ESP_LOGI(TAG, "Position reset to 0");
}

int32_t StepperMotor::mm_to_steps_256(int64_t mm) {
    // Convert millimeters to steps
    // steps = mm * steps_per_mm
    return static_cast<int32_t>(mm * steps_per_mm);
}

int32_t StepperMotor::steps_256_to_mm(int32_t steps) {
    // Convert steps to millimeters
    // mm = steps / steps_per_mm
    return static_cast<int32_t>(steps / steps_per_mm);
}

