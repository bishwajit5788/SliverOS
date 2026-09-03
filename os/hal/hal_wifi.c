/**
 * @file hal_wifi.c
 * @brief Wi-Fi HAL passive promiscuous implementation with bounded queue.
 */

#include "hal_wifi.h"
#include "hal_timer.h"
#include <string.h>

#if defined(ESP_PLATFORM)
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#endif

static hal_wifi_frame_meta_t s_queue[HAL_WIFI_QUEUE_CAPACITY];
static uint32_t s_head = 0U;
static uint32_t s_tail = 0U;
static uint32_t s_count = 0U;
static uint32_t s_dropped = 0U;
static uint32_t s_captured = 0U;
static uint8_t s_current_channel = 1U;
static bool s_promiscuous_active = false;

#if defined(ESP_PLATFORM)
static void wifi_promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (buf == NULL) {
        return;
    }

    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    if (pkt->rx_ctrl.sig_len < 24) {
        return; /* Insufficient header */
    }

    /* Fast parse 802.11 frame control field (first 2 bytes of payload) */
    const uint8_t *payload = pkt->payload;
    const uint16_t fc = (uint16_t)payload[0] | ((uint16_t)payload[1] << 8);
    const uint8_t f_type = (uint8_t)((fc >> 2) & 0x03U);
    const uint8_t f_subtype = (uint8_t)((fc >> 4) & 0x0FU);

    /* Enqueue to bounded queue */
    if (s_count >= HAL_WIFI_QUEUE_CAPACITY) {
        s_dropped++;
        return;
    }

    hal_wifi_frame_meta_t *meta = &s_queue[s_head];
    meta->timestamp_ms = hal_timer_get_ms();
    meta->rssi = pkt->rx_ctrl.rssi;
    meta->channel = pkt->rx_ctrl.channel;
    meta->length = (uint16_t)pkt->rx_ctrl.sig_len;
    meta->frame_type = f_type;
    meta->frame_subtype = f_subtype;

    s_head = (s_head + 1U) % HAL_WIFI_QUEUE_CAPACITY;
    s_count++;
    s_captured++;
}
#endif

mk_status_t hal_wifi_init(void)
{
    s_head = 0U;
    s_tail = 0U;
    s_count = 0U;
    s_dropped = 0U;
    s_captured = 0U;
    s_current_channel = 1U;
    s_promiscuous_active = false;

#if defined(ESP_PLATFORM)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        (void)nvs_flash_erase();
        (void)nvs_flash_init();
    }

    (void)esp_netif_init();
    (void)esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        return MK_STATUS_ERROR;
    }
    (void)esp_wifi_set_storage(WIFI_STORAGE_RAM);
    (void)esp_wifi_set_mode(WIFI_MODE_NULL);
    (void)esp_wifi_start();
#endif

    return MK_STATUS_OK;
}

mk_status_t hal_wifi_start_promiscuous(uint8_t initial_channel)
{
    s_current_channel = (initial_channel >= 1 && initial_channel <= 14) ? initial_channel : 1U;

#if defined(ESP_PLATFORM)
    (void)esp_wifi_set_promiscuous(false);
    (void)esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_rx_cb);
    (void)esp_wifi_set_channel(s_current_channel, WIFI_SECOND_CHAN_NONE);
    if (esp_wifi_set_promiscuous(true) != ESP_OK) {
        return MK_STATUS_ERROR;
    }
#endif

    s_promiscuous_active = true;
    return MK_STATUS_OK;
}

mk_status_t hal_wifi_stop_promiscuous(void)
{
#if defined(ESP_PLATFORM)
    (void)esp_wifi_set_promiscuous(false);
#endif
    s_promiscuous_active = false;
    return MK_STATUS_OK;
}

mk_status_t hal_wifi_set_channel(uint8_t channel)
{
    if (channel < 1 || channel > 14) {
        return MK_STATUS_INVALID_ARG;
    }

    s_current_channel = channel;
#if defined(ESP_PLATFORM)
    if (s_promiscuous_active) {
        (void)esp_wifi_set_channel(s_current_channel, WIFI_SECOND_CHAN_NONE);
    }
#endif
    return MK_STATUS_OK;
}

uint8_t hal_wifi_get_channel(void)
{
    return s_current_channel;
}

bool hal_wifi_queue_pop(hal_wifi_frame_meta_t *out_meta)
{
    if (out_meta == NULL) {
        return false;
    }

    if (s_count == 0U) {
#if !defined(ESP_PLATFORM)
        /* On host, generate occasional simulated packet for test verification */
        static uint32_t s_sim_tick = 0;
        if ((++s_sim_tick % 5) == 0) {
            out_meta->timestamp_ms = hal_timer_get_ms();
            out_meta->rssi = -65;
            out_meta->channel = s_current_channel;
            out_meta->length = 128;
            out_meta->frame_type = WIFI_FRAME_TYPE_MGMT;
            out_meta->frame_subtype = 8; /* Beacon */
            s_captured++;
            return true;
        }
#endif
        return false;
    }

    *out_meta = s_queue[s_tail];
    s_tail = (s_tail + 1U) % HAL_WIFI_QUEUE_CAPACITY;
    s_count--;
    return true;
}

uint32_t hal_wifi_get_dropped_count(void)
{
    return s_dropped;
}

uint32_t hal_wifi_get_captured_count(void)
{
    return s_captured;
}
