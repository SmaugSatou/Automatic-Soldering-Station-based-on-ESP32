/**
 * @file CMakeLists.txt
 * @brief Build configuration for web server component
 */

idf_component_register(
    SRCS "web_server.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server filesystem
)
