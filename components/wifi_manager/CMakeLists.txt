/**
 * @file CMakeLists.txt
 * @brief Build configuration for WiFi manager component
 */

idf_component_register(
    SRCS "wifi_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif
)
