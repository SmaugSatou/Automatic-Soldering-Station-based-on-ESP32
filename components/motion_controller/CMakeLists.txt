# CMakeLists.txt
# Build configuration for motion controller component

idf_component_register(
    SRCS "motion_controller.cpp"
    INCLUDE_DIRS "include"
    REQUIRES stepper_motor
)
