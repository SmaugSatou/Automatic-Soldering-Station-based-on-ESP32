/**
 * @file StepperMotor.hpp
 * @brief C++ API wrapper for stepper motor control
 * 
 * Object-oriented interface for stepper motor operations.
 * Wraps the C HAL layer providing RAII and modern C++ conveniences.
 */

#ifndef STEPPER_MOTOR_HPP
#define STEPPER_MOTOR_HPP

#include "stepper_motor_hal.h"
#include <cstdint>
#include <memory>

/**
 * @brief C++ wrapper class for stepper motor control
 */
class StepperMotor {
private:
    // @brief Current position in steps/256
    int32_t position_;
    stepper_motor_handle_t handle_;
    int32_t target_position_;
    int32_t steps_per_mm;

public:
    /**
     * @brief Constructor
     * @param config Motor configuration parameters
     * @param steps_per_mm Steps per millimeter for this motor
     */
    explicit StepperMotor(const stepper_motor_config_t& config, uint32_t steps_per_mm);
    
    /**
     * @brief Destructor
     */
    ~StepperMotor();
    
    // Delete copy constructor and assignment
    StepperMotor(const StepperMotor&) = delete;
    StepperMotor& operator=(const StepperMotor&) = delete;
    
    // Move constructor and assignment
    StepperMotor(StepperMotor&& other) noexcept;
    StepperMotor& operator=(StepperMotor&& other) noexcept;
    
    /**
     * @brief Enable or disable motor
     */
    void setEnable(bool enable);
    
    /**
     * @brief Set rotation direction
     */
    void setDirection(stepper_direction_t direction);
    
    /**
     * @brief Set microstepping mode
     */
    void setMicrostepMode(stepper_microstep_mode_t mode);
    
    /**
     * @brief Execute single step
     */
    void step();
    
    /**
     * @brief Execute multiple steps
     */
    void stepMultiple(uint32_t steps);

    /**
     * @brief Set target position in steps/256
     */
    void setTargetPosition(int32_t position);

    /**
     * @brief Execute single step towards target position
     */
    void stepToTarget();

    /**
     * @brief Execute multiple steps towards target position
     */
    void stepMultipleToTarget(uint32_t steps);

    /**
     * @brief Get current position
     */
    int32_t getPosition() const;
    
    /**
     * @brief Reset position counter(set current position to 0)
     */
    void resetPosition();
    
    /**
     * @brief Check if motor is initialized
     */
    bool isInitialized() const { return handle_ != nullptr; }

    /**
     * @brief Convert millimeters to steps/256 for current motor configuration
     */
    int32_t mm_to_steps_256(int64_t mm);

    /**
     * @brief Convert steps/256 to millimeters for current motor configuration
     */
    int32_t steps_256_to_mm(int32_t steps);
};

#endif // STEPPER_MOTOR_HPP
