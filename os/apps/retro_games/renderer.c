/**
 * @file renderer.c
 * @brief Display renderer implementation with dirty-region tracking.
 */

#include "renderer.h"
#include "hal_spi.h"
#include <string.h>
#include <stdlib.h>

static uint8_t s_framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8U];
static dirty_box_t s_dirty_box;

/* Simple 5x7 ASCII Font for numerals and uppercase characters */
static const uint8_t s_font_5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['0'] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1'] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2'] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3'] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4'] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5'] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6'] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7'] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8'] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9'] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    [':'] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['A'] = {0x7C, 0x12, 0x11, 0x12, 0x7C},
    ['B'] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C'] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D'] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F'] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G'] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H'] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I'] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['L'] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M'] = {0x7F, 0x02, 0x04, 0x02, 0x7F},
    ['N'] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O'] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P'] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['R'] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S'] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U'] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V'] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W'] = {0x7F, 0x20, 0x18, 0x20, 0x7F}
};

static void mark_dirty(uint8_t x, uint8_t y)
{
    if (!s_dirty_box.is_dirty) {
        s_dirty_box.min_x = x;
        s_dirty_box.max_x = x;
        s_dirty_box.min_y = y;
        s_dirty_box.max_y = y;
        s_dirty_box.is_dirty = true;
    } else {
        if (x < s_dirty_box.min_x) s_dirty_box.min_x = x;
        if (x > s_dirty_box.max_x) s_dirty_box.max_x = x;
        if (y < s_dirty_box.min_y) s_dirty_box.min_y = y;
        if (y > s_dirty_box.max_y) s_dirty_box.max_y = y;
    }
}

mk_status_t renderer_init(void)
{
    renderer_clear();
    return MK_STATUS_OK;
}

void renderer_clear(void)
{
    memset(s_framebuffer, 0, sizeof(s_framebuffer));
    s_dirty_box.min_x = 0;
    s_dirty_box.min_y = 0;
    s_dirty_box.max_x = DISPLAY_WIDTH - 1;
    s_dirty_box.max_y = DISPLAY_HEIGHT - 1;
    s_dirty_box.is_dirty = true;
}

void renderer_draw_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return;
    }

    uint16_t idx = x + (y / 8U) * DISPLAY_WIDTH;
    uint8_t bit = 1U << (y % 8U);

    if (color != 0) {
        s_framebuffer[idx] |= bit;
    } else {
        s_framebuffer[idx] &= ~bit;
    }

    mark_dirty(x, y);
}

void renderer_draw_line(int x0, int y0, int x1, int y1, uint8_t color)
{
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        if (x0 >= 0 && x0 < (int)DISPLAY_WIDTH && y0 >= 0 && y0 < (int)DISPLAY_HEIGHT) {
            renderer_draw_pixel((uint8_t)x0, (uint8_t)y0, color);
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void renderer_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    for (uint8_t i = 0; i < w; i++) {
        for (uint8_t j = 0; j < h; j++) {
            renderer_draw_pixel(x + i, y + j, color);
        }
    }
}

void renderer_draw_string(uint8_t x, uint8_t y, const char *str)
{
    if (str == NULL) return;

    uint8_t cur_x = x;
    while (*str && cur_x < (DISPLAY_WIDTH - 6)) {
        char c = *str++;
        if ((unsigned char)c < sizeof(s_font_5x7)/sizeof(s_font_5x7[0])) {
            const uint8_t *glyph = s_font_5x7[(unsigned char)c];
            for (uint8_t col = 0; col < 5; col++) {
                uint8_t line = glyph[col];
                for (uint8_t row = 0; row < 7; row++) {
                    if (line & (1 << row)) {
                        renderer_draw_pixel(cur_x + col, y + row, 1U);
                    }
                }
            }
        }
        cur_x += 6;
    }
}

void renderer_flush_dirty(void)
{
    if (!s_dirty_box.is_dirty) {
        return;
    }

    /* Transmit bounded dirty page regions over SPI HAL */
    uint8_t start_page = s_dirty_box.min_y / 8U;
    uint8_t end_page = s_dirty_box.max_y / 8U;

    for (uint8_t page = start_page; page <= end_page; page++) {
        uint16_t offset = page * DISPLAY_WIDTH + s_dirty_box.min_x;
        size_t len = (size_t)(s_dirty_box.max_x - s_dirty_box.min_x + 1U);
        (void)hal_spi_write_data(&s_framebuffer[offset], len);
    }

    s_dirty_box.is_dirty = false;
}

const dirty_box_t *renderer_get_dirty_box(void)
{
    return &s_dirty_box;
}
