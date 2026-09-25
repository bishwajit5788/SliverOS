/**
 * @file display_manager.c
 * @brief Unified display controller implementation with dirty-box tracking.
 */
#include "display_manager.h"
#include "hal_spi.h"
#include "hal_timer.h"
#include <string.h>

static uint8_t s_framebuffer[DISPLAY_MAX_WIDTH * DISPLAY_MAX_HEIGHT / 8U];
static mk_dirty_box_t s_dirty_box;
static mk_display_metrics_t s_metrics;
static uint64_t s_last_frame_start_us = 0U;
static uint32_t s_frame_count_sec = 0U;
static uint32_t s_sec_timestamp = 0U;

static const uint8_t s_font5x7[][5] = {
    [' ']={0,0,0,0,0},['0']={0x3E,0x51,0x49,0x45,0x3E},['1']={0,0x42,0x7F,0x40,0},['2']={0x42,0x61,0x51,0x49,0x46},['3']={0x21,0x41,0x45,0x4B,0x31},['4']={0x18,0x14,0x12,0x7F,0x10},['5']={0x27,0x45,0x45,0x45,0x39},['6']={0x3C,0x4A,0x49,0x49,0x30},['7']={1,0x71,9,5,3},['8']={0x36,0x49,0x49,0x49,0x36},['9']={6,0x49,0x49,0x29,0x1E},[':']={0,0x36,0x36,0,0},['%']={0x23,0x13,8,0x64,0x62},['-']={8,8,8,8,8},['/']={0x20,0x10,8,4,2},['>']={0,0x41,0x22,0x14,8},['<']={8,0x14,0x22,0x41,0},
    ['A']={0x7C,0x12,0x11,0x12,0x7C},['B']={0x7F,0x49,0x49,0x49,0x36},['C']={0x3E,0x41,0x41,0x41,0x22},['D']={0x7F,0x41,0x41,0x22,0x1C},['E']={0x7F,0x49,0x49,0x49,0x41},['F']={0x7F,9,9,9,1},['G']={0x3E,0x41,0x49,0x49,0x7A},['H']={0x7F,8,8,8,0x7F},['I']={0,0x41,0x7F,0x41,0},['K']={0x7F,8,0x14,0x22,0x41},['L']={0x7F,0x40,0x40,0x40,0x40},['M']={0x7F,2,4,2,0x7F},['N']={0x7F,4,8,0x10,0x7F},['O']={0x3E,0x41,0x41,0x41,0x3E},['P']={0x7F,9,9,9,6},['R']={0x7F,9,0x19,0x29,0x46},['S']={0x46,0x49,0x49,0x49,0x31},['T']={1,1,0x7F,1,1},['U']={0x3F,0x40,0x40,0x40,0x3F},['V']={0x1F,0x20,0x40,0x20,0x1F},['W']={0x7F,0x20,0x18,0x20,0x7F}
};

static void mark_dirty(uint8_t x,uint8_t y){if(!s_dirty_box.is_dirty){s_dirty_box.min_x=x;s_dirty_box.max_x=x;s_dirty_box.min_y=y;s_dirty_box.max_y=y;s_dirty_box.is_dirty=true;}else{if(x<s_dirty_box.min_x)s_dirty_box.min_x=x;if(x>s_dirty_box.max_x)s_dirty_box.max_x=x;if(y<s_dirty_box.min_y)s_dirty_box.min_y=y;if(y>s_dirty_box.max_y)s_dirty_box.max_y=y;}}
mk_status_t display_manager_init(void){memset(&s_metrics,0,sizeof(s_metrics));display_manager_clear();return MK_STATUS_OK;}
void display_manager_clear(void){memset(s_framebuffer,0,sizeof(s_framebuffer));s_dirty_box.min_x=0;s_dirty_box.min_y=0;s_dirty_box.max_x=DISPLAY_MAX_WIDTH-1;s_dirty_box.max_y=DISPLAY_MAX_HEIGHT-1;s_dirty_box.is_dirty=true;}
void display_manager_draw_pixel(uint8_t x,uint8_t y,uint8_t color){if(x>=DISPLAY_MAX_WIDTH||y>=DISPLAY_MAX_HEIGHT)return;uint16_t idx=x+(y/8U)*DISPLAY_MAX_WIDTH;uint8_t bit=(uint8_t)(1U<<(y%8U));if(color)s_framebuffer[idx]|=bit;else s_framebuffer[idx]&=(uint8_t)~bit;mark_dirty(x,y);}
void display_manager_draw_rect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t color){for(uint8_t i=0;i<w;i++){display_manager_draw_pixel(x+i,y,color);display_manager_draw_pixel(x+i,(uint8_t)(y+h-1U),color);}for(uint8_t j=0;j<h;j++){display_manager_draw_pixel(x,y+j,color);display_manager_draw_pixel((uint8_t)(x+w-1U),y+j,color);}}
void display_manager_fill_rect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t color){for(uint8_t i=0;i<w;i++)for(uint8_t j=0;j<h;j++)display_manager_draw_pixel(x+i,y+j,color);}
void display_manager_draw_string(uint8_t x,uint8_t y,const char *str){if(str==NULL)return;uint8_t cx=x;while(*str&&cx<(DISPLAY_MAX_WIDTH-6U)){unsigned char c=(unsigned char)*str++;if(c<(sizeof(s_font5x7)/sizeof(s_font5x7[0]))){const uint8_t *glyph=s_font5x7[c];for(uint8_t col=0;col<5;col++){uint8_t line=glyph[col];for(uint8_t row=0;row<7;row++)if(line&(uint8_t)(1U<<row))display_manager_draw_pixel(cx+col,y+row,1U);}}cx=(uint8_t)(cx+6U);}}

void display_manager_flush(void)
{
    if(!s_dirty_box.is_dirty)return;
    hal_spi_service();
    const uint64_t now_us=hal_timer_get_us();
    s_last_frame_start_us=now_us;
    const uint8_t start_page=(uint8_t)(s_dirty_box.min_y/8U);
    const uint8_t end_page=(uint8_t)(s_dirty_box.max_y/8U);
    const size_t len=(size_t)(s_dirty_box.max_x-s_dirty_box.min_x+1U);

    /* Queue bounded DMA transfers. If the queue is full, leave the dirty region
     * intact and retry on the next cooperative scheduler iteration. */
    for(uint8_t page=start_page;page<=end_page;page++){
        uint16_t offset=(uint16_t)(page*DISPLAY_MAX_WIDTH+s_dirty_box.min_x);
        if(hal_spi_write_data(&s_framebuffer[offset],len)!=MK_STATUS_OK)return;
    }
    hal_spi_service();
    s_dirty_box.is_dirty=false;
    s_metrics.frames_rendered++;s_frame_count_sec++;
    const uint64_t flush_end_us=hal_timer_get_us();
    s_metrics.frame_time_us=(uint32_t)(flush_end_us-now_us);
    uint32_t now_ms=(uint32_t)(now_us/1000ULL);
    if((now_ms-s_sec_timestamp)>=1000U){s_metrics.fps=s_frame_count_sec;s_frame_count_sec=0U;s_sec_timestamp=now_ms;}
}
void display_manager_get_metrics(mk_display_metrics_t *out_metrics){if(out_metrics!=NULL)*out_metrics=s_metrics;}
