/**
 * @file gcode_parser.h
 * @brief G-Code parser for soldering station commands
 *
 * Parses and validates G-Code commands for motion and soldering operations.
 *
 * SUPPORTED COMMANDS:
 * - G0 X Y Z [F] : Rapid positioning (move to position)
 * - S<amount>    : Feed solder (custom command)
 *
 * All other commands (G1, G4, G28, M104, M109, etc.) are ignored.
 * System handles homing, temperature control, and timing automatically.
 */

#ifndef GCODE_PARSER_H
#define GCODE_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief G-Code command types
 *
 * Only GCODE_CMD_MOVE (G0) and GCODE_CMD_FEED_SOLDER (S) are actively processed.
 * Other types exist for compatibility but are filtered during parsing.
 */
typedef enum {
    GCODE_CMD_NONE = 0,
    GCODE_CMD_MOVE,              // G0 - Rapid positioning (SUPPORTED)
    GCODE_CMD_FEED_SOLDER,       // S<amount> - Feed solder (SUPPORTED)
    GCODE_CMD_SET_TEMPERATURE,   // M104/M109 - Ignored (system configured)
    GCODE_CMD_HOME,              // G28 - Ignored (system handles)
    GCODE_CMD_DWELL,             // G4 - Ignored (system handles)
    GCODE_CMD_UNKNOWN
} gcode_command_type_t;

/**
 * @brief Parsed G-Code command structure
 */
typedef struct {
    gcode_command_type_t type;
    bool has_x;
    bool has_y;
    bool has_z;
    bool has_f;
    bool has_s;
    bool has_t;
    double x;
    double y;
    double z;
    double f;
    uint32_t s;
    double t;
} gcode_command_t;

/**
 * @brief G-Code parser handle
 */
typedef struct gcode_parser_handle_s* gcode_parser_handle_t;

/**
 * @brief Initialize G-Code parser
 */
gcode_parser_handle_t gcode_parser_init(void);

/**
 * @brief Deinitialize G-Code parser
 */
void gcode_parser_deinit(gcode_parser_handle_t handle);

/**
 * @brief Parse single line of G-Code
 */
bool gcode_parser_parse_line(gcode_parser_handle_t handle, const char* line, gcode_command_t* cmd);

/**
 * @brief Validate G-Code command
 */
bool gcode_parser_validate_command(gcode_parser_handle_t handle, const gcode_command_t* cmd);

/**
 * @brief Load G-Code from buffer
 */
bool gcode_parser_load_program(gcode_parser_handle_t handle, const char* program, uint32_t length);

/**
 * @brief Get next command from loaded program
 */
bool gcode_parser_get_next_command(gcode_parser_handle_t handle, gcode_command_t* cmd);

/**
 * @brief Reset parser to beginning of program
 */
void gcode_parser_reset(gcode_parser_handle_t handle);

/**
 * @brief Get current line number
 */
uint32_t gcode_parser_get_line_number(gcode_parser_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // GCODE_PARSER_H
