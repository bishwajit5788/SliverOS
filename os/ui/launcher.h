/**
 * @file launcher.h
 * @brief Graphical Application Launcher for MicroKernel OS.
 * Exposes an interactive visual carousel for the 4 applications.
 */

#ifndef MK_LAUNCHER_H
#define MK_LAUNCHER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mk_app_id_t selected_app;
    bool launch_requested;
    uint32_t animation_tick;
} mk_launcher_state_t;

mk_status_t launcher_init(void);
void launcher_render(void);
void launcher_handle_input(uint8_t input_mask);
mk_app_id_t launcher_get_selected_app(void);
bool launcher_consume_launch_request(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_LAUNCHER_H */
