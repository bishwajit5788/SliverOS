/**
 * @file hal_wifi.h
 * @brief Wi-Fi HAL for passive promiscuous frame auditing.
 * Strictly captures safe frame metadata only (RSSI, length, frame type, subtype, channel).
 * NO credential sniffing, NO PMKID capture, NO secret harvesting.
 */

#ifndef MK_HAL_WIFI_H
#define MK_HAL_WIFI_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_WIFI_QUEUE_CAPACITY 64U

typedef enum {
    WIFI_FRAME_TYPE_MGMT = 0,
    WIFI_FRAME_TYPE_CTRL = 1,
    WIFI_FRAME_TYPE_DATA = 2,
    WIFI_FRAME_TYPE_UNKNOWN = 3
} hal_wifi_frame_type_t;

/**
 * @brief Safe frame metadata payload.
 */
typedef struct {
    uint32_t timestamp_ms;
    int8_t rssi;
    uint8_t channel;
    uint16_t length;
    uint8_t frame_type;     /* Mgmt, Ctrl, Data */
    uint8_t frame_subtype;  /* Beacon, Probe, Data, ACK */
} hal_wifi_frame_meta_t;

mk_status_t hal_wifi_init(void);
mk_status_t hal_wifi_start_promiscuous(uint8_t initial_channel);
mk_status_t hal_wifi_stop_promiscuous(void);
mk_status_t hal_wifi_set_channel(uint8_t channel);
uint8_t hal_wifi_get_channel(void);

/**
 * @brief Pop next metadata element from bounded diagnostic queue.
 * Safe to call from cooperative application task.
 */
bool hal_wifi_queue_pop(hal_wifi_frame_meta_t *out_meta);

/**
 * @brief Diagnostic telemetry counters.
 */
uint32_t hal_wifi_get_dropped_count(void);
uint32_t hal_wifi_get_captured_count(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_HAL_WIFI_H */
