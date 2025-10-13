/**
 * @file CMakeLists.txt
 * @brief Build configuration for temperature sensor component
 */

idf_component_register(
    SRCS "temperature_sensor_hal.c"
         "TemperatureSensor.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
