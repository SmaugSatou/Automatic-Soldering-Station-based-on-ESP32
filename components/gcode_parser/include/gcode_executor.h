/**
 * @file gcode_executor.h
 * @brief G-Code executor for running parsed commands
 * 
 * Executes G-Code commands using motion controller and soldering iron.
 */

#ifndef GCODE_EXECUTOR_H
#define GCODE_EXECUTOR_H

#include "gcode_parser.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Executor status
 */
typedef enum {
    GCODE_EXEC_IDLE = 0,
    GCODE_EXEC_RUNNING,
    GCODE_EXEC_PAUSED,
    GCODE_EXEC_COMPLETE,
    GCODE_EXEC_ERROR
} gcode_executor_status_t;

/**
 * @brief G-Code executor handle
 */
typedef struct gcode_executor_handle_s* gcode_executor_handle_t;

/**
 * @brief Initialize G-Code executor
 */
gcode_executor_handle_t gcode_executor_init(void);

/**
 * @brief Deinitialize G-Code executor
 */
void gcode_executor_deinit(gcode_executor_handle_t handle);

/**
 * @brief Start executing G-Code program
 */
bool gcode_executor_start(gcode_executor_handle_t handle);

/**
 * @brief Pause execution
 */
void gcode_executor_pause(gcode_executor_handle_t handle);

/**
 * @brief Resume execution
 */
void gcode_executor_resume(gcode_executor_handle_t handle);

/**
 * @brief Stop execution
 */
void gcode_executor_stop(gcode_executor_handle_t handle);

/**
 * @brief Get executor status
 */
gcode_executor_status_t gcode_executor_get_status(gcode_executor_handle_t handle);

/**
 * @brief Get progress percentage
 */
uint8_t gcode_executor_get_progress(gcode_executor_handle_t handle);

/**
 * @brief Get estimated time remaining in seconds
 */
uint32_t gcode_executor_get_time_remaining(gcode_executor_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // GCODE_EXECUTOR_H
