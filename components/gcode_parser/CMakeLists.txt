/**
 * @file CMakeLists.txt
 * @brief Build configuration for G-Code parser component
 */

idf_component_register(
    SRCS "gcode_parser.c"
         "gcode_executor.cpp"
    INCLUDE_DIRS "include"
    REQUIRES motion_controller soldering_iron
)
