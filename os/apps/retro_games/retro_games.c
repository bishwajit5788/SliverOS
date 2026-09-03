/**
 * @file retro_games.c
 * @brief Space Micro-Lander retro game application implementation.
 */

#include "retro_games.h"
#include "renderer.h"
#include "input.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LANDING_PAD_X       50
#define LANDING_PAD_Y       60
#define LANDING_PAD_W       28

static game_lander_t s_lander;
static uint32_t s_frame_counter = 0U;
static bool s_running = false;

static void reset_lander(void)
{
    s_lander.x = 20 * 10;   /* Fixed point (x10) */
    s_lander.y = 5 * 10;
    s_lander.vx = 5;
    s_lander.vy = 0;
    s_lander.fuel = 500;
    s_lander.status = GAME_STATE_PLAYING;
}

mk_status_t retro_games_init(void)
{
    (void)renderer_init();
    (void)input_init();
    reset_lander();
    s_lander.score = 0;
    s_running = false;
    return MK_STATUS_OK;
}

mk_status_t retro_games_start(void)
{
    s_running = true;
    reset_lander();
    return MK_STATUS_OK;
}

mk_status_t retro_games_stop(void)
{
    s_running = false;
    return MK_STATUS_OK;
}

void retro_games_task(void *context)
{
    (void)context;
    if (!s_running) {
        return;
    }

    s_frame_counter++;
    uint8_t input = input_get_state();

    if (s_lander.status == GAME_STATE_PLAYING) {
        /* Apply gravity */
        s_lander.vy += 1;

        /* Apply player controls */
        if (s_lander.fuel > 0) {
            if ((input & INPUT_BTN_UP) || (input & INPUT_BTN_A)) {
                s_lander.vy -= 2;
                s_lander.fuel--;
            }
            if (input & INPUT_BTN_LEFT) {
                s_lander.vx -= 1;
                s_lander.fuel--;
            }
            if (input & INPUT_BTN_RIGHT) {
                s_lander.vx += 1;
                s_lander.fuel--;
            }
        }

        /* Update positions */
        s_lander.x += s_lander.vx;
        s_lander.y += s_lander.vy;

        int px = s_lander.x / 10;
        int py = s_lander.y / 10;

        /* Check landing boundary */
        if (py >= LANDING_PAD_Y - 4) {
            if (px >= LANDING_PAD_X && (px + 6) <= (LANDING_PAD_X + LANDING_PAD_W) && s_lander.vy < 8 && abs(s_lander.vx) < 4) {
                s_lander.status = GAME_STATE_LANDED;
                s_lander.score += 100U + s_lander.fuel;
            } else {
                s_lander.status = GAME_STATE_CRASHED;
            }
        }
    } else {
        /* Reset on Button A / B if finished */
        if ((input & INPUT_BTN_A) || (input & INPUT_BTN_B)) {
            reset_lander();
        }
    }

    /* Rendering step: Erase dirty region and redraw scene */
    renderer_clear();

    /* Draw HUD */
    char hud_buf[32];
    snprintf(hud_buf, sizeof(hud_buf), "F:%u S:%u", s_lander.fuel, s_lander.score);
    renderer_draw_string(2, 2, hud_buf);

    /* Draw Landing Platform */
    renderer_draw_line(LANDING_PAD_X, LANDING_PAD_Y, LANDING_PAD_X + LANDING_PAD_W, LANDING_PAD_Y, 1U);

    /* Draw Lander Sprite (6x5 pixel polygon) */
    int lx = s_lander.x / 10;
    int ly = s_lander.y / 10;
    if (lx >= 0 && lx < (int)DISPLAY_WIDTH - 6 && ly >= 0 && ly < (int)DISPLAY_HEIGHT - 6) {
        if (s_lander.status == GAME_STATE_CRASHED) {
            /* Debris particles */
            renderer_draw_pixel((uint8_t)(lx - 2), (uint8_t)(ly - 2), 1U);
            renderer_draw_pixel((uint8_t)(lx + 6), (uint8_t)(ly - 1), 1U);
            renderer_draw_pixel((uint8_t)(lx + 2), (uint8_t)(ly + 4), 1U);
        } else {
            renderer_draw_line(lx + 2, ly, lx + 4, ly, 1U);
            renderer_draw_line(lx, ly + 4, lx + 6, ly + 4, 1U);
            renderer_draw_line(lx, ly + 4, lx + 3, ly + 1, 1U);
            renderer_draw_line(lx + 6, ly + 4, lx + 3, ly + 1, 1U);
        }
    }

    if (s_lander.status == GAME_STATE_LANDED) {
        renderer_draw_string(40, 24, "LANDED");
    } else if (s_lander.status == GAME_STATE_CRASHED) {
        renderer_draw_string(38, 24, "CRASHED");
    }

    /* Flush dirty region over SPI HAL */
    renderer_flush_dirty();
}

void retro_games_get_lander(game_lander_t *out_lander)
{
    if (out_lander != NULL) {
        *out_lander = s_lander;
    }
}

mk_status_t retro_games_reset(void)
{
    reset_lander();
    s_lander.score = 0;
    return MK_STATUS_OK;
}

static void retro_games_handle_event(const mk_event_t *event)
{
    (void)event;
}

static const mk_app_interface_t s_retro_games_interface = {
    .id = MK_APP_RETRO_GAMES,
    .name = "RETRO_GAMES",
    .init = retro_games_init,
    .start = retro_games_start,
    .tick = retro_games_task,
    .handle_event = retro_games_handle_event,
    .stop = retro_games_stop,
    .reset = retro_games_reset
};

const mk_app_interface_t *retro_games_get_interface(void)
{
    return &s_retro_games_interface;
}
