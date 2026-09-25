/**
 * @file host_protocol.h
 * @brief SliverOS Host UI Protocol (SLVR/1) Specification & Framing Interface.
 *
 * Provides a deterministic, robust binary framing protocol over Native USB Serial/JTAG.
 * Supports bidirectional frame exchange between ESP32-S3 SliverOS runtime and Mac Web Host.
 *
 * Framing Format:
 * [MAGIC: 4B ("SLVR")] [VERSION: 1B (0x01)] [TYPE: 1B] [FLAGS: 1B] [SEQ: 1B] [LEN: 2B (BE)] [PAYLOAD: N bytes] [CRC16: 2B (BE)]
 */

#ifndef SLVR_HOST_PROTOCOL_H
#define SLVR_HOST_PROTOCOL_H

#include "kernel_types.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol Identification */
#define SLVR_MAGIC_0            'S'
#define SLVR_MAGIC_1            'L'
#define SLVR_MAGIC_2            'V'
#define SLVR_MAGIC_3            'R'
#define SLVR_MAGIC_U32          0x52564C53U /* "SLVR" in little-endian / wire order: 'S','L','V','R' */
#define SLVR_VERSION_1          0x01U

/* Protocol Sizing Constants */
#define SLVR_HEADER_LEN         10U  /* Magic(4) + Ver(1) + Type(1) + Flags(1) + Seq(1) + Len(2) */
#define SLVR_CRC_LEN            2U   /* CRC-16-CCITT */
#define SLVR_FRAME_OVERHEAD     (SLVR_HEADER_LEN + SLVR_CRC_LEN) /* 12 bytes */
#define SLVR_MAX_PAYLOAD_LEN    512U
#define SLVR_MAX_FRAME_LEN      (SLVR_FRAME_OVERHEAD + SLVR_MAX_PAYLOAD_LEN) /* 524 bytes */

/* Flags */
#define SLVR_FLAG_NONE          0x00U
#define SLVR_FLAG_ACK_REQ       0x01U
#define SLVR_FLAG_IS_RESPONSE   0x02U
#define SLVR_FLAG_ERROR         0x04U

/* =========================================================================
 * Message Types: ESP32 -> Web Host (0x01 - 0x7F)
 * ========================================================================= */
typedef enum {
    SLVR_MSG_NONE               = 0x00,
    SLVR_MSG_HELLO              = 0x01,
    SLVR_MSG_DEVICE_INFO        = 0x02,
    SLVR_MSG_READY              = 0x03,
    SLVR_MSG_DESKTOP_STATE      = 0x04,
    SLVR_MSG_APP_STATE          = 0x05,
    SLVR_MSG_DEVICE_STATUS      = 0x06,
    SLVR_MSG_MEMORY_STATUS      = 0x07,
    SLVR_MSG_STORAGE_STATUS     = 0x08,
    SLVR_MSG_LOG                = 0x09,
    SLVR_MSG_ERROR              = 0x0A,
    SLVR_MSG_EVENT              = 0x0B,
    SLVR_MSG_PONG               = 0x0C,
    SLVR_MSG_GAME_STATE         = 0x0D,
    SLVR_MSG_TERMINAL_OUTPUT    = 0x0E
} slvr_msg_type_t;

/* =========================================================================
 * Command Types: Web Host -> ESP32 (0x81 - 0xFF)
 * ========================================================================= */
typedef enum {
    SLVR_CMD_CONNECT            = 0x81,
    SLVR_CMD_GET_INFO           = 0x82,
    SLVR_CMD_GET_STATUS         = 0x83,
    SLVR_CMD_LAUNCH_APP         = 0x84,
    SLVR_CMD_EXIT_APP           = 0x85,
    SLVR_CMD_APP_INPUT          = 0x86,
    SLVR_CMD_KEY_EVENT          = 0x87,
    SLVR_CMD_BUTTON_EVENT       = 0x88,
    SLVR_CMD_NAVIGATION         = 0x89,
    SLVR_CMD_TERMINAL_INPUT     = 0x8A,
    SLVR_CMD_GET_LOGS           = 0x8B,
    SLVR_CMD_RESET              = 0x8C,
    SLVR_CMD_PING               = 0x8D
} slvr_cmd_type_t;

/* Parsed Frame Representation */
typedef struct {
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint8_t sequence;
    uint16_t length;
    uint8_t payload[SLVR_MAX_PAYLOAD_LEN];
    uint16_t crc16;
} slvr_frame_t;

/* Stream Parser State Machine */
typedef enum {
    SLVR_PARSE_STATE_MAGIC_0 = 0,
    SLVR_PARSE_STATE_MAGIC_1,
    SLVR_PARSE_STATE_MAGIC_2,
    SLVR_PARSE_STATE_MAGIC_3,
    SLVR_PARSE_STATE_VERSION,
    SLVR_PARSE_STATE_TYPE,
    SLVR_PARSE_STATE_FLAGS,
    SLVR_PARSE_STATE_SEQ,
    SLVR_PARSE_STATE_LEN_H,
    SLVR_PARSE_STATE_LEN_L,
    SLVR_PARSE_STATE_PAYLOAD,
    SLVR_PARSE_STATE_CRC_H,
    SLVR_PARSE_STATE_CRC_L
} slvr_parser_state_t;

typedef struct {
    slvr_parser_state_t state;
    slvr_frame_t current_frame;
    uint16_t payload_idx;
    uint32_t dropped_bytes;
    uint32_t frames_received;
    uint32_t crc_errors;
} slvr_parser_t;

/* =========================================================================
 * Structured Payloads
 * ========================================================================= */

#pragma pack(push, 1)

typedef struct {
    char product[24];
    char version[12];
    char build_id[24];
    char git_revision[24];
    char chip_family[16];
    uint32_t chip_revision;
    uint32_t flash_size_bytes;
    uint32_t psram_size_bytes;
    uint32_t internal_sram_total;
    uint32_t internal_sram_free;
    uint32_t psram_total;
    uint32_t psram_free;
} slvr_payload_device_info_t;

typedef struct {
    uint8_t id;
    uint8_t state;
    uint8_t priority;
    uint8_t padding;
    uint32_t period_ticks;
    uint32_t execution_count;
    uint32_t last_execution_us;
    uint32_t worst_execution_us;
    uint32_t overrun_count;
    char name[12];
} slvr_task_telemetry_t;

typedef struct {
    uint32_t tick;
    uint32_t uptime_seconds;
    uint32_t scheduler_iterations;
    uint8_t active_app;
    uint8_t kernel_state;
    uint8_t task_count;
    uint8_t fault_count;
    slvr_task_telemetry_t tasks[8];
} slvr_payload_device_status_t;

typedef struct {
    uint32_t arena_capacity;
    uint32_t arena_used;
    uint32_t arena_free;
    uint32_t arena_peak;
    uint32_t allocation_count;
    uint32_t free_count;
    uint32_t failed_allocations;
    uint32_t psram_total;
    uint32_t psram_free;
} slvr_payload_memory_status_t;

typedef struct {
    uint32_t total_sectors;
    uint32_t free_sectors;
    uint32_t active_sectors;
    uint32_t obsolete_sectors;
    uint32_t record_commit_count;
} slvr_payload_storage_status_t;

typedef struct {
    int16_t lander_x;   /* Fixed point (x10) */
    int16_t lander_y;   /* Fixed point (x10) */
    int16_t lander_vx;
    int16_t lander_vy;
    uint16_t fuel;
    uint16_t score;
    uint8_t status;     /* 0 = PLAYING, 1 = LANDED, 2 = CRASHED */
    uint8_t pad_x;
    uint8_t pad_y;
    uint8_t pad_w;
    uint32_t frame_seq;
} slvr_payload_game_state_t;

typedef struct {
    uint8_t app_id;
    uint8_t app_state;
    uint16_t runs_completed;
    uint16_t error_count;
    uint16_t data_len;
    uint8_t data[128];
} slvr_payload_app_state_t;

typedef struct {
    uint8_t level;
    uint32_t timestamp_ms;
    char text[120];
} slvr_payload_log_t;

typedef struct {
    uint16_t text_len;
    char text[256];
} slvr_payload_terminal_t;

#pragma pack(pop)

/* Protocol Functions */
uint16_t slvr_crc16(const uint8_t *data, size_t len);
void slvr_parser_init(slvr_parser_t *parser);
bool slvr_parser_feed_byte(slvr_parser_t *parser, uint8_t byte, slvr_frame_t *out_frame);
mk_status_t slvr_encode_frame(const slvr_frame_t *frame, uint8_t *out_buf,
                             size_t max_buf_len, size_t *out_frame_len);

#ifdef __cplusplus
}
#endif

#endif /* SLVR_HOST_PROTOCOL_H */
