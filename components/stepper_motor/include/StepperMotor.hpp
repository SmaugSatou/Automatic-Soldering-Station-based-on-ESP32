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
public:
    /**
     * @brief Constructor
     * @param config Motor configuration parameters
     */
    explicit StepperMotor(const stepper_motor_config_t& config);
    
    /**
     * @brief Destructor
     */
    ~StepperMotor();
    
    StepperMotor(const StepperMotor&) = delete;
    StepperMotor& operator=(const StepperMotor&) = delete;
    
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
     * @brief Get current position
     */
    int32_t getPosition() const;
    
    /**
     * @brief Reset position counter
     */
    void resetPosition();
    
    /**
     * @brief Check if motor is initialized
     */
    bool isInitialized() const { return handle_ != nullptr; }

private:
    stepper_motor_handle_t handle_;
};

#endif // STEPPER_MOTOR_HPP
