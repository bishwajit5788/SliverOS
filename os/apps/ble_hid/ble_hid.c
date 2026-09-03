/**
 * @file ble_hid.c
 * @brief BLE HID keyboard application implementation.
 */

#include "ble_hid.h"
#include "macro_parser.h"
#include "hal_ble.h"
#include "vfs.h"
#include "fault_manager.h"
#include "state_machine.h"
#include "kernel.h"
#include <string.h>

typedef enum {
    BLE_APP_STATE_UNINIT = 0,
    BLE_APP_STATE_WAIT_CONNECTION,
    BLE_APP_STATE_OPEN_FILE,
    BLE_APP_STATE_PARSE,
    BLE_APP_STATE_RELEASE,
    BLE_APP_STATE_DELAY,
    BLE_APP_STATE_COOLDOWN
} ble_app_internal_state_t;

static ble_app_internal_state_t s_state = BLE_APP_STATE_UNINIT;
static macro_parser_t s_parser;
static int32_t s_macro_fd = -1;
static uint32_t s_delay_remaining_ticks = 0U;

mk_status_t ble_hid_init(void)
{
    s_state = BLE_APP_STATE_WAIT_CONNECTION;
    s_macro_fd = -1;
    s_delay_remaining_ticks = 0U;
    return MK_STATUS_OK;
}

mk_status_t ble_hid_start(void)
{
    mk_status_t st = hal_ble_start_advertising();
    if (st == MK_STATUS_OK) {
        s_state = BLE_APP_STATE_WAIT_CONNECTION;
    }
    return st;
}

mk_status_t ble_hid_stop(void)
{
    if (s_macro_fd >= 0) {
        (void)vfs_close(s_macro_fd);
        s_macro_fd = -1;
    }
    (void)hal_ble_stop_advertising();
    s_state = BLE_APP_STATE_WAIT_CONNECTION;
    return MK_STATUS_OK;
}

void ble_hid_task(void *context)
{
    (void)context;

    switch (s_state) {
        case BLE_APP_STATE_WAIT_CONNECTION:
            if (hal_ble_is_connected()) {
                s_state = BLE_APP_STATE_OPEN_FILE;
            }
            break;

        case BLE_APP_STATE_OPEN_FILE: {
            mk_status_t st = vfs_open("macro.txt", VFS_O_RDONLY, &s_macro_fd);
            if (st == MK_STATUS_OK) {
                (void)macro_parser_init(&s_parser, s_macro_fd);
                s_state = BLE_APP_STATE_PARSE;
            } else {
                /* Macro file not found or unreadable; cool down */
                s_delay_remaining_ticks = 50U;
                s_state = BLE_APP_STATE_COOLDOWN;
            }
            break;
        }

        case BLE_APP_STATE_PARSE: {
            macro_event_t evt;
            mk_status_t st = macro_parser_next_event(&s_parser, &evt);
            if (st != MK_STATUS_OK || evt.type == MACRO_CMD_EOF || evt.type == MACRO_CMD_ERROR) {
                if (s_macro_fd >= 0) {
                    (void)vfs_close(s_macro_fd);
                    s_macro_fd = -1;
                }
                s_delay_remaining_ticks = 100U; /* Cooldown 100 ticks */
                s_state = BLE_APP_STATE_COOLDOWN;
                break;
            }

            if (evt.type == MACRO_CMD_STRING_CHAR || evt.type == MACRO_CMD_ENTER || evt.type == MACRO_CMD_KEY) {
                if (evt.keycode != HAL_BLE_KEY_NONE) {
                    (void)hal_ble_send_key(evt.modifier, evt.keycode);
                    s_state = BLE_APP_STATE_RELEASE;
                }
            } else if (evt.type == MACRO_CMD_DELAY) {
                s_delay_remaining_ticks = (evt.delay_ms / 10U) + 1U;
                s_state = BLE_APP_STATE_DELAY;
            }
            break;
        }

        case BLE_APP_STATE_RELEASE:
            (void)hal_ble_send_release();
            s_state = BLE_APP_STATE_PARSE;
            break;

        case BLE_APP_STATE_DELAY:
            if (s_delay_remaining_ticks > 0U) {
                s_delay_remaining_ticks--;
            } else {
                s_state = BLE_APP_STATE_PARSE;
            }
            break;

        case BLE_APP_STATE_COOLDOWN:
            if (s_delay_remaining_ticks > 0U) {
                s_delay_remaining_ticks--;
            } else {
                s_state = BLE_APP_STATE_WAIT_CONNECTION;
            }
            break;

        default:
            s_state = BLE_APP_STATE_WAIT_CONNECTION;
            break;
    }
}

static void ble_hid_handle_event(const mk_event_t *event)
{
    if (event == NULL) return;
    if (event->type == MK_EVENT_BLE_CONNECTED) {
        s_state = BLE_APP_STATE_OPEN_FILE;
    } else if (event->type == MK_EVENT_BLE_DISCONNECTED) {
        s_state = BLE_APP_STATE_WAIT_CONNECTION;
    }
}

static const mk_app_interface_t s_ble_hid_interface = {
    .id = MK_APP_BLE_HID,
    .name = "BLE_HID",
    .init = ble_hid_init,
    .start = ble_hid_start,
    .tick = ble_hid_task,
    .handle_event = ble_hid_handle_event,
    .stop = ble_hid_stop,
    .reset = ble_hid_init
};

const mk_app_interface_t *ble_hid_get_interface(void)
{
    return &s_ble_hid_interface;
}
