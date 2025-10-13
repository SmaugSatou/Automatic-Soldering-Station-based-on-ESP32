/**
 * @file stepper_motor_hal.h
 * @brief Hardware Abstraction Layer for DRV8825 stepper motor driver
 * 
 * Provides low-level C interface for controlling stepper motors via DRV8825 driver.
 * Supports microstepping modes (1/2, 1/4, 1/8, 1/16, 1/32) for precision control.
 * Used for X, Y, Z, and solder supply (S) axis control.
 */

#ifndef STEPPER_MOTOR_HAL_H
#define STEPPER_MOTOR_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Microstepping mode configuration
 */
typedef enum {
    STEPPER_MICROSTEP_FULL = 0,
    STEPPER_MICROSTEP_1_2,
    STEPPER_MICROSTEP_1_4,
    STEPPER_MICROSTEP_1_8,
    STEPPER_MICROSTEP_1_16,
    STEPPER_MICROSTEP_1_32
} stepper_microstep_mode_t;

/**
 * @brief Motor rotation direction
 */
typedef enum {
    STEPPER_DIR_FORWARD = 0,
    STEPPER_DIR_BACKWARD
} stepper_direction_t;

/**
 * @brief Stepper motor configuration structure
 */
typedef struct {
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    gpio_num_t enable_pin;
    gpio_num_t mode0_pin;
    gpio_num_t mode1_pin;
    gpio_num_t mode2_pin;
    stepper_microstep_mode_t microstep_mode;
    uint32_t steps_per_revolution;
} stepper_motor_config_t;

/**
 * @brief Stepper motor handle
 */
typedef struct stepper_motor_handle_s* stepper_motor_handle_t;

/**
 * @brief Initialize stepper motor driver
 */
stepper_motor_handle_t stepper_motor_hal_init(const stepper_motor_config_t* config);

/**
 * @brief Deinitialize stepper motor driver
 */
void stepper_motor_hal_deinit(stepper_motor_handle_t handle);

/**
 * @brief Enable or disable the motor
 */
void stepper_motor_hal_set_enable(stepper_motor_handle_t handle, bool enable);

/**
 * @brief Set motor direction
 */
void stepper_motor_hal_set_direction(stepper_motor_handle_t handle, stepper_direction_t direction);

/**
 * @brief Set microstepping mode
 */
void stepper_motor_hal_set_microstep_mode(stepper_motor_handle_t handle, stepper_microstep_mode_t mode);

/**
 * @brief Execute single step
 */
void stepper_motor_hal_step(stepper_motor_handle_t handle);

/**
 * @brief Execute multiple steps
 */
void stepper_motor_hal_step_multiple(stepper_motor_handle_t handle, uint32_t steps);

/**
 * @brief Get current position in steps
 */
int32_t stepper_motor_hal_get_position(stepper_motor_handle_t handle);

/**
 * @brief Reset position counter to zero
 */
void stepper_motor_hal_reset_position(stepper_motor_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // STEPPER_MOTOR_HAL_H
