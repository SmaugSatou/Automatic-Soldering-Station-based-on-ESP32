/**
 * @file CMakeLists.txt
 * @brief Build configuration for soldering iron component
 */

idf_component_register(
    SRCS "soldering_iron_hal.c"
         "SolderingIron.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
