# CMakeLists.txt
# Build configuration for soldering iron component

idf_component_register(
    SRCS "soldering_iron_hal.c"
         "SolderingIron.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_ledc esp_driver_gpio
)
