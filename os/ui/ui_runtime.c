/**
 * @file ui_runtime.c
 * @brief Graphical runtime coordinator with explicit application lifecycle ownership.
 */
#include "ui_runtime.h"
#include "display_manager.h"
#include "launcher.h"
#include "kernel.h"
#include "memory_manager.h"
#include "memory_pool.h"
#include "fault_manager.h"
#include "apps/retro_games/input.h"
#include <stdio.h>
#include <string.h>

static mk_ui_mode_t s_mode = UI_MODE_LAUNCHER;
static uint8_t s_dev_screen_page = 0U;
static mk_app_id_t s_active_app = MK_APP_BLE_HID;

mk_status_t ui_runtime_init(void)
{
    (void)display_manager_init();
    (void)launcher_init();
    s_mode = UI_MODE_LAUNCHER;
    s_dev_screen_page = 0U;
    s_active_app = MK_APP_BLE_HID;
    return MK_STATUS_OK;
}

static void render_dev_screen(void)
{
    display_manager_clear();
    mk_kernel_t *k = mk_kernel_get_instance();
    mk_mem_stats_t mem; mk_memory_get_stats(&mem);
    mk_diag_identity_t diag; mk_kernel_get_diag_identity(&diag);
    mk_display_metrics_t disp; display_manager_get_metrics(&disp);
    char buf[32];
    if (s_dev_screen_page == 0U) {
        display_manager_fill_rect(0, 0, DISPLAY_MAX_WIDTH, 9, 1U);
        display_manager_draw_string(2, 1, "SYSTEM DIAG - 1/2");
        snprintf(buf, sizeof(buf), "K: %s T:%u", mk_kernel_state_to_str(k->state), k->tick);
        display_manager_draw_string(2, 12, buf);
        snprintf(buf, sizeof(buf), "ARENA: %u/%uK", (unsigned)(mem.used_bytes / 1024), (unsigned)(mem.total_capacity / 1024));
        display_manager_draw_string(2, 22, buf);
        snprintf(buf, sizeof(buf), "PEAK: %uK FAIL:%u", (unsigned)(mem.peak_used_bytes / 1024), mem.failed_allocations);
        display_manager_draw_string(2, 32, buf);
        display_manager_draw_string(2, 42, "PSRAM: 8MB (EXT)");
        display_manager_draw_string(2, 54, "[A] NEXT  [B] EXIT");
    } else {
        display_manager_fill_rect(0, 0, DISPLAY_MAX_WIDTH, 9, 1U);
        display_manager_draw_string(2, 1, "SYSTEM DIAG - 2/2");
        snprintf(buf, sizeof(buf), "%s R%u FLASH:%uM", diag.chip_family, diag.chip_revision, diag.flash_size_bytes / (1024*1024));
        display_manager_draw_string(2, 12, buf);
        snprintf(buf, sizeof(buf), "FPS: %u FT:%uus", disp.fps, disp.frame_time_us);
        display_manager_draw_string(2, 22, buf);
        mk_fault_record_t fault;
        if (mk_fault_get_latest(&fault) == MK_STATUS_OK)
            snprintf(buf, sizeof(buf), "FLT:%u %s", fault.fault_id, mk_fault_source_str(fault.source));
        else snprintf(buf, sizeof(buf), "FAULTS: NONE");
        display_manager_draw_string(2, 34, buf);
        display_manager_draw_string(2, 54, "[A] PREV  [B] EXIT");
    }
    display_manager_flush();
}

static void exit_active_app(void)
{
    mk_kernel_t *k = mk_kernel_get_instance();
    mk_app_control_t *app = mk_kernel_get_app(s_active_app);
    if (app != NULL && app->state == MK_APP_STATE_ACTIVE) {
        /* Lifecycle is explicit: ACTIVE -> STOPPING -> READY. The app task
         * must observe STOPPING and release owned resources before READY. */
        (void)mk_app_transition(k, s_active_app, MK_APP_STATE_STOPPING);
        (void)mk_app_transition(k, s_active_app, MK_APP_STATE_READY);
    }
    s_mode = UI_MODE_LAUNCHER;
}

void ui_runtime_task(void *context)
{
    (void)context;
    uint8_t input = input_get_state();

    /* B is a UI navigation event; it is not allowed to bypass app lifecycle. */
    if (s_mode == UI_MODE_DEV_SCREEN) {
        if (input & INPUT_BTN_A) s_dev_screen_page = (s_dev_screen_page == 0U) ? 1U : 0U;
        if (input & INPUT_BTN_B) s_mode = UI_MODE_LAUNCHER;
        render_dev_screen();
        return;
    }

    if (s_mode == UI_MODE_LAUNCHER) {
        if (input & INPUT_BTN_B) { s_mode = UI_MODE_DEV_SCREEN; render_dev_screen(); return; }
        launcher_handle_input(input);
        launcher_render();
        if (launcher_consume_launch_request()) {
            s_active_app = launcher_get_selected_app();
            mk_kernel_t *k = mk_kernel_get_instance();
            (void)mk_app_transition(k, s_active_app, MK_APP_STATE_ACTIVE);
            s_mode = UI_MODE_APP_ACTIVE;
        }
        return;
    }

    if (s_mode == UI_MODE_APP_ACTIVE && (input & INPUT_BTN_B)) {
        exit_active_app();
    }
}

void ui_runtime_set_mode(mk_ui_mode_t mode) { s_mode = mode; }
mk_ui_mode_t ui_runtime_get_mode(void) { return s_mode; }
