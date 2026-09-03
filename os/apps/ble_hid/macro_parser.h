/**
 * @file macro_parser.h
 * @brief Incremental bounded-memory macro file parser.
 * Never loads arbitrarily large files into RAM; parses from a 64-byte streaming buffer.
 */

#ifndef MK_MACRO_PARSER_H
#define MK_MACRO_PARSER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MACRO_BUFFER_SIZE 64U

typedef enum {
    MACRO_CMD_NONE = 0,
    MACRO_CMD_KEY,
    MACRO_CMD_STRING_CHAR,
    MACRO_CMD_DELAY,
    MACRO_CMD_ENTER,
    MACRO_CMD_EOF,
    MACRO_CMD_ERROR
} macro_cmd_type_t;

typedef struct {
    macro_cmd_type_t type;
    uint8_t modifier;
    uint8_t keycode;
    uint32_t delay_ms;
    char ch;
} macro_event_t;

typedef struct {
    int32_t fd;
    char buffer[MACRO_BUFFER_SIZE];
    size_t buf_len;
    size_t buf_pos;
    bool eof_reached;
    uint32_t line_number;
    char current_str[32];
    size_t str_pos;
    size_t str_len;
} macro_parser_t;

mk_status_t macro_parser_init(macro_parser_t *parser, int32_t fd);
mk_status_t macro_parser_next_event(macro_parser_t *parser, macro_event_t *out_event);

#ifdef __cplusplus
}
#endif

#endif /* MK_MACRO_PARSER_H */
