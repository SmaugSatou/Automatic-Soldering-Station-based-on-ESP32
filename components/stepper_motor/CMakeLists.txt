/**
 * @file CMakeLists.txt
 * @brief Build configuration for stepper motor component
 */

idf_component_register(
    SRCS "stepper_motor_hal.c"
         "StepperMotor.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
