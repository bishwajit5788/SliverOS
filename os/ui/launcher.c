/**
 * @file launcher.c
 * @brief Graphical Application Launcher implementation.
 */

#include "launcher.h"
#include "display_manager.h"
#include "apps/retro_games/input.h"
#include <string.h>

static mk_launcher_state_t s_launcher;

static const char *s_app_titles[MK_APP_COUNT] = {
    "1. BLE-HID MACRO",
    "2. WIFI DIAGNOSTIC",
    "3. NETWORK DIAG",
    "4. RETRO GAMES"
};

static const char *s_app_descs[MK_APP_COUNT] = {
    "USB Keyboard Emulation",
    "Safe Promiscuous RX",
    "ICMP & TCP 22/80/443",
    "Space Micro-Lander"
};

mk_status_t launcher_init(void)
{
    s_launcher.selected_app = MK_APP_BLE_HID;
    s_launcher.launch_requested = false;
    s_launcher.animation_tick = 0U;
    return MK_STATUS_OK;
}

void launcher_handle_input(uint8_t input)
{
    if (input & INPUT_BTN_RIGHT) {
        s_launcher.selected_app = (mk_app_id_t)((s_launcher.selected_app + 1U) % MK_APP_COUNT);
    }
    if (input & INPUT_BTN_LEFT) {
        s_launcher.selected_app = (mk_app_id_t)((s_launcher.selected_app + MK_APP_COUNT - 1U) % MK_APP_COUNT);
    }
    if (input & INPUT_BTN_A) {
        s_launcher.launch_requested = true;
    }
}

void launcher_render(void)
{
    s_launcher.animation_tick++;
    display_manager_clear();

    /* Header Bar */
    display_manager_fill_rect(0, 0, DISPLAY_MAX_WIDTH, 10, 1U);
    display_manager_draw_string(2, 1, "MICROKERNEL OS");

    /* App Selector Carousel Box */
    display_manager_draw_rect(4, 16, DISPLAY_MAX_WIDTH - 8, 36, 1U);

    /* Arrow indicators */
    display_manager_draw_string(8, 28, "<");
    display_manager_draw_string(DISPLAY_MAX_WIDTH - 14, 28, ">");

    /* Current App Title */
    display_manager_draw_string(18, 22, s_app_titles[s_launcher.selected_app]);
    display_manager_draw_string(18, 34, s_app_descs[s_launcher.selected_app]);

    /* Footer Prompt */
    display_manager_draw_string(12, 54, "[A] LAUNCH  [B] DIAG");

    display_manager_flush();
}

mk_app_id_t launcher_get_selected_app(void)
{
    return s_launcher.selected_app;
}

bool launcher_consume_launch_request(void)
{
    bool req = s_launcher.launch_requested;
    s_launcher.launch_requested = false;
    return req;
}
