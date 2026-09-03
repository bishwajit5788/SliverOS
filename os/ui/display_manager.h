/**
 * @file display_manager.h
 * @brief Unified display controller abstraction for ST7789 and SSD1306.
 * Implements dirty-region tracking, frame timing, and PSRAM/SRAM buffer management.
 */

#ifndef MK_DISPLAY_MANAGER_H
#define MK_DISPLAY_MANAGER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_MAX_WIDTH   128U
#define DISPLAY_MAX_HEIGHT  64U

typedef struct {
    uint8_t min_x;
    uint8_t min_y;
    uint8_t max_x;
    uint8_t max_y;
    bool is_dirty;
} mk_dirty_box_t;

typedef struct {
    uint32_t fps;
    uint32_t frame_time_us;
    uint32_t frames_rendered;
} mk_display_metrics_t;

mk_status_t display_manager_init(void);
void display_manager_clear(void);
void display_manager_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void display_manager_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void display_manager_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void display_manager_draw_string(uint8_t x, uint8_t y, const char *str);
void display_manager_flush(void);
void display_manager_get_metrics(mk_display_metrics_t *out_metrics);

#ifdef __cplusplus
}
#endif

#endif /* MK_DISPLAY_MANAGER_H */
