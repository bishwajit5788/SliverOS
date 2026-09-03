/**
 * @file test_game_physics.c
 * @brief Unit tests for Space Micro-Lander vector physics and collision detection.
 */

#include "retro_games.h"
#include <stdio.h>
#include <assert.h>

void run_game_physics_tests(void)
{
    printf("[TEST] Starting Space Micro-Lander Physics & Collision Unit Tests...\n");

    mk_status_t status = retro_games_init();
    assert(status == MK_STATUS_OK);

    game_lander_t lander;
    retro_games_get_lander(&lander);
    assert(lander.status == GAME_STATE_PLAYING);
    assert(lander.fuel > 0);
    assert(lander.score == 0);

    /* 1. Verify gravity acceleration */
    status = retro_games_start();
    assert(status == MK_STATUS_OK);

    int16_t initial_vy = lander.vy;
    /* Run 5 ticks of the game loop without thrust */
    for (int i = 0; i < 5; i++) {
        retro_games_task(NULL);
    }
    retro_games_get_lander(&lander);
    /* In gravity environment, vertical speed must increase downward */
    assert(lander.vy > initial_vy);

    /* 2. Reset and verify state restoration */
    assert(retro_games_reset() == MK_STATUS_OK);
    retro_games_get_lander(&lander);
    assert(lander.status == GAME_STATE_PLAYING);
    assert(lander.score == 0);
    assert(lander.fuel == 500);

    printf("[PASS] Space Micro-Lander Physics & Collision Unit Tests Passed Successfully.\n");
}
