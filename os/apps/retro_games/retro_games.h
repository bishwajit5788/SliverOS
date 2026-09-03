/**
 * @file retro_games.h
 * @brief Low-priority Space Micro-Lander retro game application.
 */

#ifndef MK_APP_RETRO_GAMES_H
#define MK_APP_RETRO_GAMES_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_STATE_PLAYING = 0,
    GAME_STATE_LANDED,
    GAME_STATE_CRASHED
} game_status_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t vx;
    int16_t vy;
    uint16_t fuel;
    uint16_t score;
    game_status_t status;
} game_lander_t;

mk_status_t retro_games_init(void);
mk_status_t retro_games_start(void);
mk_status_t retro_games_stop(void);
void retro_games_task(void *context);
void retro_games_get_lander(game_lander_t *out_lander);

#ifdef __cplusplus
}
#endif

#endif /* MK_APP_RETRO_GAMES_H */
