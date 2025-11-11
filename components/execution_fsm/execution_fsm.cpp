/**
 * @file execution_fsm.cpp
 * @brief Implementation of execution sub-FSM
 */

#include "execution_fsm.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "StepperMotor.hpp"
#include <string.h>
#include <cmath>

static const char *TAG = "EXEC_FSM";

extern StepperMotor* motor_x;
extern StepperMotor* motor_y;
extern StepperMotor* motor_z;
extern StepperMotor* motor_s;

static const char* state_names[] = {
    "IDLE",
    "MOVE_TO_POINT",
    "MOVE_DOWN",
    "SOLDERING",
    "MOVE_UP",
    "RETURN_HOME",
    "COMPLETE"
};

static inline uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void transition_to_state(execution_sub_fsm_t* fsm, exec_sub_state_t new_state) {
    if (fsm->sub_state != new_state) {
        ESP_LOGI(TAG, "Transition: %s -> %s",
                 state_names[fsm->sub_state],
                 state_names[new_state]);

        fsm->sub_state = new_state;
        fsm->state_enter_time = get_time_ms();
        fsm->operation_in_progress = false;
    }
}

execution_config_t exec_sub_fsm_get_default_config(void) {
    execution_config_t config = {
        .safe_z_height = 16000,
        .soldering_z_height = 18000,
        .home_x = 0,
        .home_y = 0,
        .home_z = 0
    };
    return config;
}

void exec_sub_fsm_init(execution_sub_fsm_t* fsm, const execution_config_t* config) {
    memset(fsm, 0, sizeof(execution_sub_fsm_t));
    fsm->sub_state = EXEC_STATE_IDLE;
    fsm->state_enter_time = get_time_ms();

    if (config) {
        fsm->config = *config;
    } else {
        fsm->config = exec_sub_fsm_get_default_config();
    }

    ESP_LOGI(TAG, "Init: safe_z=%ld, solder_z=%ld, home=(%ld,%ld,%ld)",
             fsm->config.safe_z_height, fsm->config.soldering_z_height,
             fsm->config.home_x, fsm->config.home_y, fsm->config.home_z);
}

void exec_sub_fsm_process(execution_sub_fsm_t* fsm, const solder_point_t* points, int num_points) {
    switch (fsm->sub_state) {

        case EXEC_STATE_IDLE:
            if (fsm->current_point_index < num_points) {
                transition_to_state(fsm, EXEC_STATE_MOVE_TO_POINT);
            } else {
                transition_to_state(fsm, EXEC_STATE_RETURN_HOME);
            }
            break;

        case EXEC_STATE_MOVE_TO_POINT: {
            const solder_point_t* target = &points[fsm->current_point_index];

            if (!fsm->operation_in_progress) {
                ESP_LOGI(TAG, "Point %d/%d: X=%ld Y=%ld Z=%ld",
                         fsm->current_point_index + 1, num_points,
                         target->x, target->y, fsm->config.safe_z_height);

                motor_x->setTargetPosition(target->x);
                motor_y->setTargetPosition(target->y);
                motor_z->setTargetPosition(fsm->config.safe_z_height);

                fsm->operation_in_progress = true;
            }

            bool x_reached = (motor_x->getPosition() == target->x);
            bool y_reached = (motor_y->getPosition() == target->y);
            bool z_safe = (motor_z->getPosition() == fsm->config.safe_z_height);

            if (!x_reached || !y_reached || !z_safe) {
                if (!x_reached) {
                    motor_x->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_x->getPosition() - motor_x->getTargetPosition())));
                }
                if (!y_reached) {
                    motor_y->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_y->getPosition() - motor_y->getTargetPosition())));
                }
                if (!z_safe) {
                    motor_z->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_z->getPosition() - motor_z->getTargetPosition())));
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(200));
                transition_to_state(fsm, EXEC_STATE_MOVE_DOWN);
            }
            break;
        }

        case EXEC_STATE_MOVE_DOWN: {
            if (!fsm->operation_in_progress) {
                motor_z->setTargetPosition(fsm->config.soldering_z_height);
                fsm->operation_in_progress = true;
            }

            bool z_reached = (motor_z->getPosition() == fsm->config.soldering_z_height);

            if (!z_reached) {
                motor_z->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_z->getPosition() - motor_z->getTargetPosition())));
            } else {
                transition_to_state(fsm, EXEC_STATE_SOLDERING);
            }
            break;
        }

        case EXEC_STATE_SOLDERING: {
            const solder_point_t* solder_pos = &points[fsm->current_point_index];
            uint32_t solder_duration = solder_pos->solder_time_ms;

            if (!fsm->operation_in_progress) {
                motor_s->setEnable(true);

                int32_t feed_amount = solder_duration / 10;
                fsm->solder_start_pos = motor_s->getPosition();
                motor_s->setTargetPosition(fsm->solder_start_pos + feed_amount);
                fsm->solder_start_time = get_time_ms();

                fsm->operation_in_progress = true;
            }

            uint32_t elapsed = get_time_ms() - fsm->solder_start_time;

            if (elapsed < solder_duration) {
                if (motor_s->getPosition() != motor_s->getTargetPosition()) {
                    motor_s->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_s->getPosition() - motor_s->getTargetPosition())));
                }
            } else {
                int32_t feed_amount = motor_s->getPosition() - fsm->solder_start_pos;
                ESP_LOGI(TAG, "Soldered: %ld steps fed in %.1fs",
                         feed_amount, solder_duration / 1000.0f);
                motor_s->setEnable(false);

                fsm->solder_points_completed++;
                transition_to_state(fsm, EXEC_STATE_MOVE_UP);
            }
            break;
        }

        case EXEC_STATE_MOVE_UP: {
            if (!fsm->operation_in_progress) {
                motor_z->setTargetPosition(fsm->config.safe_z_height);
                fsm->operation_in_progress = true;
            }

            bool z_reached = (motor_z->getPosition() == fsm->config.safe_z_height);

            if (!z_reached) {
                motor_z->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_z->getPosition() - motor_z->getTargetPosition())));
            } else {
                fsm->current_point_index++;

                if (fsm->current_point_index < num_points) {
                    transition_to_state(fsm, EXEC_STATE_MOVE_TO_POINT);
                } else {
                    transition_to_state(fsm, EXEC_STATE_RETURN_HOME);
                }
            }
            break;
        }

        case EXEC_STATE_RETURN_HOME: {
            if (!fsm->operation_in_progress) {
                ESP_LOGI(TAG, "Returning to home (%ld,%ld,%ld)",
                         fsm->config.home_x, fsm->config.home_y, fsm->config.home_z);
                motor_x->setTargetPosition(fsm->config.home_x);
                motor_y->setTargetPosition(fsm->config.home_y);
                motor_z->setTargetPosition(fsm->config.home_z);
                fsm->operation_in_progress = true;
            }

            bool x_home = (motor_x->getPosition() == fsm->config.home_x);
            bool y_home = (motor_y->getPosition() == fsm->config.home_y);
            bool z_home = (motor_z->getPosition() == fsm->config.home_z);

            if (!x_home || !y_home || !z_home) {
                if (!x_home) {
                    motor_x->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_x->getPosition())));
                }
                if (!y_home) {
                    motor_y->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_y->getPosition())));
                }
                if (!z_home) {
                    motor_z->stepMultipleToTarget(static_cast<uint32_t>(std::abs(motor_z->getPosition())));
                }
            } else {
                transition_to_state(fsm, EXEC_STATE_COMPLETE);
            }
            break;
        }

        case EXEC_STATE_COMPLETE:
            break;

        default:
            ESP_LOGW(TAG, "Unknown state: %d", fsm->sub_state);
            break;
    }
}

exec_sub_state_t exec_sub_fsm_get_state(const execution_sub_fsm_t* fsm) {
    return fsm->sub_state;
}

int exec_sub_fsm_get_completed_count(const execution_sub_fsm_t* fsm) {
    return fsm->solder_points_completed;
}

const char* exec_sub_fsm_get_state_name(exec_sub_state_t state) {
    if (state < EXEC_STATE_COUNT) {
        return state_names[state];
    }
    return "UNKNOWN";
}
