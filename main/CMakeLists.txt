# CMakeLists.txt
# Build configuration for main application

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash
        wifi_manager
        web_server
        filesystem
        stepper_motor
        soldering_iron
        temperature_sensor
        motion_controller
        gcode_parser
        display
)
