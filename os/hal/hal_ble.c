/**
 * @file hal_ble.c
 * @brief BLE HID keyboard HAL using ESP-IDF esp_hid + NimBLE.
 */
#include "hal_ble.h"
#include <string.h>

#if defined(ESP_PLATFORM)
#include "esp_hid.h"
#include "esp_hidd.h"
#include "esp_hid_common.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_hs_adv.h"
#endif

static bool s_ble_initialized = false;
static bool s_ble_connected = false;
static bool s_ble_advertising = false;

#if defined(ESP_PLATFORM)
static esp_hidd_dev_t *s_hid_dev = NULL;
static uint8_t s_own_addr_type = 0U;

static const uint8_t s_keyboard_report_map[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01,
    0x85, 0x01,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7,
    0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65,
    0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00,
    0xC0
};

static esp_hid_raw_report_map_t s_report_maps[] = {
    { .data = s_keyboard_report_map, .len = sizeof(s_keyboard_report_map) }
};

static esp_hid_device_config_t s_hid_config = {
    .vendor_id = 0x16C0,
    .product_id = 0x27DB,
    .version = 0x0100,
    .device_name = "SliverOS Keyboard",
    .manufacturer_name = "SliverOS",
    .serial_number = "SLIVEROS-001",
    .report_maps = s_report_maps,
    .report_maps_len = 1
};

static void ble_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)s_hid_config.device_name;
    fields.name_len = (uint8_t)strlen(s_hid_config.device_name);
    fields.name_is_complete = 1;
    fields.appearance = 0x03C1; /* HID keyboard */
    fields.appearance_is_present = 1;
    if (ble_gap_adv_set_fields(&fields) != 0) return;

    struct ble_gap_adv_params params;
    memset(&params, 0, sizeof(params));
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    if (ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &params, NULL, NULL) == 0) {
        s_ble_advertising = true;
    }
}

static void ble_on_sync(void)
{
    if (ble_hs_id_infer_auto(0, &s_own_addr_type) != 0) return;
    if (s_hid_dev != NULL) ble_advertise();
}

static void nimble_host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void hid_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *data = (esp_hidd_event_data_t *)event_data;
    switch (event) {
        case ESP_HIDD_START_EVENT:
            ble_advertise();
            break;
        case ESP_HIDD_CONNECT_EVENT:
            s_ble_connected = true;
            s_ble_advertising = false;
            break;
        case ESP_HIDD_DISCONNECT_EVENT:
            s_ble_connected = false;
            s_ble_advertising = false;
            (void)data;
            ble_advertise();
            break;
        case ESP_HIDD_STOP_EVENT:
            s_ble_connected = false;
            s_ble_advertising = false;
            break;
        default:
            break;
    }
}
#endif

mk_status_t hal_ble_init(void)
{
    s_ble_initialized = false;
    s_ble_connected = false;
    s_ble_advertising = false;
#if defined(ESP_PLATFORM)
    if (nimble_port_init() != ESP_OK) return MK_STATUS_ERROR;
    ble_hs_cfg.sync_cb = ble_on_sync;
    if (esp_hidd_dev_init(&s_hid_config, ESP_HID_TRANSPORT_BLE, hid_event_handler, &s_hid_dev) != ESP_OK) {
        return MK_STATUS_ERROR;
    }
    nimble_port_freertos_init(nimble_host_task);
#endif
    s_ble_initialized = true;
    return MK_STATUS_OK;
}

mk_status_t hal_ble_start_advertising(void)
{
    if (!s_ble_initialized) return MK_STATUS_INVALID_STATE;
#if defined(ESP_PLATFORM)
    if (!s_ble_connected) ble_advertise();
#else
    s_ble_advertising = true;
#endif
    return MK_STATUS_OK;
}

mk_status_t hal_ble_stop_advertising(void)
{
#if defined(ESP_PLATFORM)
    if (s_ble_advertising) (void)ble_gap_adv_stop();
#endif
    s_ble_advertising = false;
    return MK_STATUS_OK;
}

bool hal_ble_is_connected(void)
{
    return s_ble_connected;
}

mk_status_t hal_ble_send_report(const hal_ble_hid_report_t *report)
{
    if (report == NULL) return MK_STATUS_INVALID_ARG;
    if (!hal_ble_is_connected()) return MK_STATUS_BUSY;
#if defined(ESP_PLATFORM)
    if (s_hid_dev == NULL) return MK_STATUS_INVALID_STATE;
    uint8_t buffer[8] = {0};
    buffer[0] = report->modifiers;
    for (size_t i = 0; i < 6U; ++i) buffer[2U + i] = report->keycodes[i];
    return (esp_hidd_dev_input_set(s_hid_dev, 0U, 1U, buffer, sizeof(buffer)) == ESP_OK) ? MK_STATUS_OK : MK_STATUS_ERROR;
#else
    return MK_STATUS_OK;
#endif
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
