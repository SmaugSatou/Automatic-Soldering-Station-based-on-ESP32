/**
 * @file motion_controller.h
 * @brief Motion controller for coordinated multi-axis movement
 * 
 * Manages X, Y, Z, and solder supply motors for synchronized movements.
 * Implements movement planning and trajectory control.
 */

#ifndef MOTION_CONTROLLER_H
#define MOTION_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 3D position structure
 */
typedef struct {
    double x;
    double y;
    double z;
} position_t;

/**
 * @brief Motion controller configuration
 */
typedef struct {
    double max_velocity_x;
    double max_velocity_y;
    double max_velocity_z;
    double max_acceleration;
    double steps_per_mm_x;
    double steps_per_mm_y;
    double steps_per_mm_z;
    double work_area_x_max;
    double work_area_y_max;
    double work_area_z_max;
} motion_controller_config_t;

/**
 * @brief Motion controller handle
 */
typedef struct motion_controller_handle_s* motion_controller_handle_t;

/**
 * @brief Initialize motion controller
 */
motion_controller_handle_t motion_controller_init(const motion_controller_config_t* config);

/**
 * @brief Deinitialize motion controller
 */
void motion_controller_deinit(motion_controller_handle_t handle);

/**
 * @brief Move to absolute position
 */
bool motion_controller_move_to(motion_controller_handle_t handle, const position_t* target);

/**
 * @brief Get current position
 */
position_t motion_controller_get_position(motion_controller_handle_t handle);

/**
 * @brief Home all axes
 */
bool motion_controller_home(motion_controller_handle_t handle);

/**
 * @brief Check if position is within work area
 */
bool motion_controller_is_position_valid(motion_controller_handle_t handle, const position_t* pos);

/**
 * @brief Emergency stop
 */
void motion_controller_emergency_stop(motion_controller_handle_t handle);

/**
 * @brief Check if movement is in progress
 */
bool motion_controller_is_moving(motion_controller_handle_t handle);

/**
 * @brief Feed solder material
 */
void motion_controller_feed_solder(motion_controller_handle_t handle, uint32_t steps);

#ifdef __cplusplus
}
#endif

#endif // MOTION_CONTROLLER_H
