/**
 * @file renderer.h
 * @brief Display renderer abstraction with dirty-region tracking.
 */

#ifndef MK_RENDERER_H
#define MK_RENDERER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_WIDTH   128U
#define DISPLAY_HEIGHT  64U

typedef struct {
    uint8_t min_x;
    uint8_t min_y;
    uint8_t max_x;
    uint8_t max_y;
    bool is_dirty;
} dirty_box_t;

mk_status_t renderer_init(void);
void renderer_clear(void);
void renderer_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void renderer_draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void renderer_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void renderer_draw_string(uint8_t x, uint8_t y, const char *str);
void renderer_flush_dirty(void);
const dirty_box_t *renderer_get_dirty_box(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_RENDERER_H */
