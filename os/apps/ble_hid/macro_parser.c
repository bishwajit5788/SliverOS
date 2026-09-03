/**
 * @file macro_parser.c
 * @brief Incremental bounded-memory macro file parser implementation.
 */

#include "macro_parser.h"
#include "vfs.h"
#include "hal_ble.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

mk_status_t macro_parser_init(macro_parser_t *parser, int32_t fd)
{
    if (parser == NULL || fd < 0) {
        return MK_STATUS_INVALID_ARG;
    }

    memset(parser, 0, sizeof(macro_parser_t));
    parser->fd = fd;
    parser->line_number = 1U;
    return MK_STATUS_OK;
}

static bool refill_buffer(macro_parser_t *p)
{
    if (p->eof_reached) {
        return false;
    }

    size_t bytes_read = 0;
    mk_status_t st = vfs_read(p->fd, p->buffer, MACRO_BUFFER_SIZE, &bytes_read);
    if (st != MK_STATUS_OK || bytes_read == 0) {
        p->eof_reached = true;
        p->buf_len = 0;
        p->buf_pos = 0;
        return false;
    }

    p->buf_len = bytes_read;
    p->buf_pos = 0;
    return true;
}

static int get_next_char(macro_parser_t *p)
{
    if (p->buf_pos >= p->buf_len) {
        if (!refill_buffer(p)) {
            return -1;
        }
    }
    return (unsigned char)p->buffer[p->buf_pos++];
}

static uint8_t char_to_hid(char c, uint8_t *out_mod)
{
    *out_mod = HAL_BLE_MOD_NONE;
    if (c >= 'a' && c <= 'z') {
        return (uint8_t)(HAL_BLE_KEY_A + (c - 'a'));
    }
    if (c >= 'A' && c <= 'Z') {
        *out_mod = HAL_BLE_MOD_LSHIFT;
        return (uint8_t)(HAL_BLE_KEY_A + (c - 'A'));
    }
    if (c == ' ') {
        return HAL_BLE_KEY_SPACE;
    }
    if (c == '\n' || c == '\r') {
        return HAL_BLE_KEY_ENTER;
    }
    return HAL_BLE_KEY_NONE;
}

mk_status_t macro_parser_next_event(macro_parser_t *p, macro_event_t *out_event)
{
    if (p == NULL || out_event == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    memset(out_event, 0, sizeof(macro_event_t));

    /* If we are currently streaming characters from a STRING token */
    if (p->str_pos < p->str_len) {
        char ch = p->current_str[p->str_pos++];
        out_event->type = MACRO_CMD_STRING_CHAR;
        out_event->ch = ch;
        out_event->keycode = char_to_hid(ch, &out_event->modifier);
        out_event->delay_ms = 20U;
        return MK_STATUS_OK;
    }

    /* Read next line command */
    char word[16];
    size_t wlen = 0;
    int c;

    /* Skip leading whitespace */
    while ((c = get_next_char(p)) != -1) {
        if (c == '\n') {
            p->line_number++;
        } else if (!isspace(c)) {
            word[wlen++] = (char)c;
            break;
        }
    }

    if (c == -1) {
        out_event->type = MACRO_CMD_EOF;
        return MK_STATUS_OK;
    }

    /* Read command token */
    while ((c = get_next_char(p)) != -1 && wlen < (sizeof(word) - 1)) {
        if (isspace(c)) {
            if (c == '\n') p->line_number++;
            break;
        }
        word[wlen++] = (char)c;
    }
    word[wlen] = '\0';

    if (strcmp(word, "STRING") == 0) {
        /* Read rest of line into current_str */
        p->str_len = 0;
        p->str_pos = 0;
        /* skip space */
        while ((c = get_next_char(p)) != -1 && (c == ' ' || c == '\t')) {}
        if (c != -1 && c != '\n' && c != '\r') {
            p->current_str[p->str_len++] = (char)c;
            while ((c = get_next_char(p)) != -1 && c != '\n' && c != '\r') {
                if (p->str_len < (sizeof(p->current_str) - 1)) {
                    p->current_str[p->str_len++] = (char)c;
                }
            }
        }
        if (c == '\n') p->line_number++;

        if (p->str_len > 0) {
            char ch = p->current_str[p->str_pos++];
            out_event->type = MACRO_CMD_STRING_CHAR;
            out_event->ch = ch;
            out_event->keycode = char_to_hid(ch, &out_event->modifier);
            out_event->delay_ms = 20U;
            return MK_STATUS_OK;
        }
    } else if (strcmp(word, "DELAY") == 0) {
        char num[10];
        size_t nlen = 0;
        while ((c = get_next_char(p)) != -1 && isspace(c)) {
            if (c == '\n') p->line_number++;
        }
        if (c != -1) {
            num[nlen++] = (char)c;
            while ((c = get_next_char(p)) != -1 && isdigit(c) && nlen < (sizeof(num) - 1)) {
                num[nlen++] = (char)c;
            }
        }
        num[nlen] = '\0';
        out_event->type = MACRO_CMD_DELAY;
        out_event->delay_ms = (uint32_t)atoi(num);
        return MK_STATUS_OK;
    } else if (strcmp(word, "ENTER") == 0) {
        out_event->type = MACRO_CMD_ENTER;
        out_event->modifier = HAL_BLE_MOD_NONE;
        out_event->keycode = HAL_BLE_KEY_ENTER;
        out_event->delay_ms = 50U;
        return MK_STATUS_OK;
    }

    out_event->type = MACRO_CMD_NONE;
    return MK_STATUS_OK;
}
