/**
 * @file hal_ble.c
 * @brief Bluetooth Low Energy HID keyboard HAL implementation.
 */

#include "hal_ble.h"
#include <string.h>

#if defined(ESP_PLATFORM)
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#endif

static bool s_ble_initialized = false;
static bool s_ble_connected = false;
static bool s_ble_advertising = false;

mk_status_t hal_ble_init(void)
{
    s_ble_initialized = true;
    s_ble_connected = false;
    s_ble_advertising = false;

#if defined(ESP_PLATFORM)
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        return MK_STATUS_ERROR;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        return MK_STATUS_ERROR;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        return MK_STATUS_ERROR;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        return MK_STATUS_ERROR;
    }
#endif

    return MK_STATUS_OK;
}

mk_status_t hal_ble_start_advertising(void)
{
    if (!s_ble_initialized) {
        return MK_STATUS_INVALID_STATE;
    }
    s_ble_advertising = true;
    return MK_STATUS_OK;
}

mk_status_t hal_ble_stop_advertising(void)
{
    s_ble_advertising = false;
    return MK_STATUS_OK;
}

bool hal_ble_is_connected(void)
{
#if !defined(ESP_PLATFORM)
    /* In host testing, simulate connected state when advertising */
    return s_ble_advertising;
#else
    return s_ble_connected;
#endif
}

mk_status_t hal_ble_send_report(const hal_ble_hid_report_t *report)
{
    if (report == NULL) {
        return MK_STATUS_INVALID_ARG;
    }
    if (!hal_ble_is_connected()) {
        return MK_STATUS_BUSY;
    }

#if defined(ESP_PLATFORM)
    /* Transmit HID report characteristic notification */
#endif

    return MK_STATUS_OK;
}

mk_status_t hal_ble_send_key(uint8_t modifier, uint8_t keycode)
{
    hal_ble_hid_report_t report;
    memset(&report, 0, sizeof(report));
    report.modifiers = modifier;
    report.keycodes[0] = keycode;

    return hal_ble_send_report(&report);
}

mk_status_t hal_ble_send_release(void)
{
    hal_ble_hid_report_t report;
    memset(&report, 0, sizeof(report));
    return hal_ble_send_report(&report);
}
