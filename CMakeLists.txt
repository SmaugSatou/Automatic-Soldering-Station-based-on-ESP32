# CMakeLists.txt
# Root CMake configuration for Automatic Soldering Station
# ESP-IDF project build configuration

cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Require esp_littlefs library
get_filename_component(configName "${CMAKE_BINARY_DIR}" NAME)
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/.pio/libdeps/${configName}/esp_littlefs")


project(automatic_soldering_station)
