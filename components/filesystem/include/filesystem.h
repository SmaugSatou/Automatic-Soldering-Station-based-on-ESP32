/**
 * @file filesystem.h
 * @brief LittleFS filesystem manager for web files
 * 
 * Manages LittleFS partition for storing HTML, CSS, JS files.
 * Provides file access for web server.
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Filesystem configuration
 */
typedef struct {
    const char* partition_label;
    const char* base_path;
    uint32_t max_files;
} filesystem_config_t;

/**
 * @brief Filesystem handle
 */
typedef struct filesystem_handle_s* filesystem_handle_t;

/**
 * @brief Initialize filesystem
 */
filesystem_handle_t filesystem_init(const filesystem_config_t* config);

/**
 * @brief Deinitialize filesystem
 */
void filesystem_deinit(filesystem_handle_t handle);

/**
 * @brief Check if file exists
 */
bool filesystem_file_exists(filesystem_handle_t handle, const char* path);

/**
 * @brief Get file size
 */
int32_t filesystem_get_file_size(filesystem_handle_t handle, const char* path);

/**
 * @brief Read entire file into buffer
 */
int32_t filesystem_read_file(filesystem_handle_t handle, const char* path, 
                             uint8_t* buffer, uint32_t buffer_size);

/**
 * @brief Write buffer to file
 */
bool filesystem_write_file(filesystem_handle_t handle, const char* path, 
                           const uint8_t* data, uint32_t size);

/**
 * @brief Delete file
 */
bool filesystem_delete_file(filesystem_handle_t handle, const char* path);

/**
 * @brief Get filesystem usage information
 */
bool filesystem_get_usage(filesystem_handle_t handle, uint32_t* total, uint32_t* used);

#ifdef __cplusplus
}
#endif

#endif // FILESYSTEM_H
