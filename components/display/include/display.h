/**
 * @file display.h
 * @brief OLED display driver for status information
 * 
 * Optional component for displaying process information on OLED screen.
 * Shows position, temperature, time, and progress.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display configuration
 */
typedef struct {
    uint8_t i2c_address;
    int32_t sda_pin;
    int32_t scl_pin;
    uint32_t i2c_frequency;
    uint16_t width;
    uint16_t height;
} display_config_t;

/**
 * @brief Display handle
 */
typedef struct display_handle_s* display_handle_t;

/**
 * @brief Initialize display
 */
display_handle_t display_init(const display_config_t* config);

/**
 * @brief Deinitialize display
 */
void display_deinit(display_handle_t handle);

/**
 * @brief Clear display
 */
void display_clear(display_handle_t handle);

/**
 * @brief Update display with current status
 */
void display_update_status(display_handle_t handle, double x, double y, double z, 
                          double temperature, uint32_t time_remaining);

/**
 * @brief Show text message
 */
void display_show_message(display_handle_t handle, const char* message);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
