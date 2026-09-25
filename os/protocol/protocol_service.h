/**
 * @file protocol_service.h
 * @brief SliverOS Host Protocol Service for Native USB Serial/JTAG.
 *
 * Runs as a high-priority cooperative task that drains USB Serial/JTAG I/O,
 * parses SLVR/1 frames, executes commands, and broadcasts rate-limited telemetry.
 */

#ifndef SLVR_PROTOCOL_SERVICE_H
#define SLVR_PROTOCOL_SERVICE_H

#include "kernel_types.h"
#include "host_protocol.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

mk_status_t protocol_service_init(void);
void protocol_service_task(void *context);

/* Outbound Messaging API */
mk_status_t protocol_service_send_frame(const slvr_frame_t *frame);
mk_status_t protocol_service_send_log(uint8_t level, const char *msg);
mk_status_t protocol_service_send_terminal(const char *text);
mk_status_t protocol_service_broadcast_game_state(const slvr_payload_game_state_t *game);

/* Status & Diagnostics */
bool protocol_service_is_connected(void);
uint32_t protocol_service_get_rx_frame_count(void);
uint32_t protocol_service_get_tx_frame_count(void);

/* Host Unit Test & Simulation Injection API */
#if !defined(ESP_PLATFORM) || defined(MK_HOST_TEST)
void protocol_service_inject_rx_bytes(const uint8_t *bytes, size_t len);
size_t protocol_service_pop_tx_bytes(uint8_t *out_buf, size_t max_len);
void protocol_service_reset_mock(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SLVR_PROTOCOL_SERVICE_H */
