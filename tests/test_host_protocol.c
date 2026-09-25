/**
 * @file test_host_protocol.c
 * @brief Host unit test suite for SLVR/1 framing, CRC16, parser, and protocol service.
 */

#include "host_protocol.h"
#include "protocol_service.h"
#include "kernel.h"
#include "retro_games.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_host_protocol(void)
{
    printf("[TEST] Starting SliverOS Host Protocol (SLVR/1) Unit Tests...\n");

    /* 1. Verify CRC16-CCITT Determinism */
    const uint8_t test_vec[] = "123456789";
    uint16_t crc = slvr_crc16(test_vec, 9U);
    /* Standard CRC-16/CCITT-FALSE across "123456789" is 0x29B1 */
    assert(crc == 0x29B1U);

    /* 2. Encode a valid frame and parse it */
    slvr_frame_t frame_tx;
    memset(&frame_tx, 0, sizeof(frame_tx));
    frame_tx.version = SLVR_VERSION_1;
    frame_tx.type = SLVR_CMD_GET_INFO;
    frame_tx.flags = SLVR_FLAG_NONE;
    frame_tx.sequence = 42U;
    frame_tx.length = 4U;
    frame_tx.payload[0] = 0xAA;
    frame_tx.payload[1] = 0xBB;
    frame_tx.payload[2] = 0xCC;
    frame_tx.payload[3] = 0xDD;

    uint8_t wire_buf[SLVR_MAX_FRAME_LEN];
    size_t wire_len = 0U;
    mk_status_t status = slvr_encode_frame(&frame_tx, wire_buf, sizeof(wire_buf), &wire_len);
    assert(status == MK_STATUS_OK);
    assert(wire_len == SLVR_FRAME_OVERHEAD + 4U);
    assert(wire_buf[0] == 'S' && wire_buf[1] == 'L' && wire_buf[2] == 'V' && wire_buf[3] == 'R');
    assert(wire_buf[4] == SLVR_VERSION_1);
    assert(wire_buf[5] == SLVR_CMD_GET_INFO);

    /* 3. Feed byte-by-byte into parser */
    slvr_parser_t parser;
    slvr_parser_init(&parser);
    slvr_frame_t frame_rx;
    bool frame_ready = false;

    for (size_t i = 0; i < wire_len; i++) {
        frame_ready = slvr_parser_feed_byte(&parser, wire_buf[i], &frame_rx);
        if (i < wire_len - 1) {
            assert(!frame_ready);
        }
    }
    assert(frame_ready);
    assert(frame_rx.version == SLVR_VERSION_1);
    assert(frame_rx.type == SLVR_CMD_GET_INFO);
    assert(frame_rx.sequence == 42U);
    assert(frame_rx.length == 4U);
    assert(memcmp(frame_rx.payload, frame_tx.payload, 4U) == 0);

    /* 4. Malformed Frame Rejection - Corrupted CRC */
    wire_buf[wire_len - 1] ^= 0xFF; /* Corrupt last byte of CRC */
    slvr_parser_init(&parser);
    frame_ready = false;
    for (size_t i = 0; i < wire_len; i++) {
        if (slvr_parser_feed_byte(&parser, wire_buf[i], &frame_rx)) {
            frame_ready = true;
        }
    }
    assert(!frame_ready);
    assert(parser.crc_errors > 0);

    /* 5. Noise Resynchronization */
    /* Prepend 20 bytes of garbage, then append the valid frame */
    uint8_t noisy_stream[256];
    memset(noisy_stream, 0x55, 20);
    wire_buf[wire_len - 1] ^= 0xFF; /* Restore CRC */
    memcpy(&noisy_stream[20], wire_buf, wire_len);

    slvr_parser_init(&parser);
    frame_ready = false;
    for (size_t i = 0; i < 20 + wire_len; i++) {
        if (slvr_parser_feed_byte(&parser, noisy_stream[i], &frame_rx)) {
            frame_ready = true;
        }
    }
    assert(frame_ready);
    assert(frame_rx.type == SLVR_CMD_GET_INFO);

    /* 6. Protocol Service Command Handshake Integration */
    assert(protocol_service_init() == MK_STATUS_OK);
    protocol_service_reset_mock();

    /* Send SLVR_CMD_CONNECT from host */
    slvr_frame_t connect_cmd;
    memset(&connect_cmd, 0, sizeof(connect_cmd));
    connect_cmd.version = SLVR_VERSION_1;
    connect_cmd.type = SLVR_CMD_CONNECT;
    connect_cmd.sequence = 1U;
    connect_cmd.length = 0U;

    uint8_t connect_raw[SLVR_MAX_FRAME_LEN];
    size_t connect_len = 0U;
    assert(slvr_encode_frame(&connect_cmd, connect_raw, sizeof(connect_raw), &connect_len) == MK_STATUS_OK);

    protocol_service_inject_rx_bytes(connect_raw, connect_len);
    protocol_service_task(NULL);

    assert(protocol_service_is_connected());

    /* Verify response from service (should be SLVR_MSG_HELLO followed by info/status) */
    uint8_t resp_buf[1024];
    size_t resp_len = protocol_service_pop_tx_bytes(resp_buf, sizeof(resp_buf));
    assert(resp_len >= SLVR_FRAME_OVERHEAD);
    assert(resp_buf[0] == 'S' && resp_buf[1] == 'L' && resp_buf[2] == 'V' && resp_buf[3] == 'R');

    /* 7. Terminal Command Execution via Host Protocol */
    slvr_frame_t term_cmd;
    memset(&term_cmd, 0, sizeof(term_cmd));
    term_cmd.version = SLVR_VERSION_1;
    term_cmd.type = SLVR_CMD_TERMINAL_INPUT;
    term_cmd.sequence = 2U;
    const char *cmd_str = "help";
    term_cmd.length = (uint16_t)strlen(cmd_str);
    memcpy(term_cmd.payload, cmd_str, term_cmd.length);

    uint8_t term_raw[SLVR_MAX_FRAME_LEN];
    size_t term_len = 0U;
    assert(slvr_encode_frame(&term_cmd, term_raw, sizeof(term_raw), &term_len) == MK_STATUS_OK);

    protocol_service_inject_rx_bytes(term_raw, term_len);
    protocol_service_task(NULL);

    resp_len = protocol_service_pop_tx_bytes(resp_buf, sizeof(resp_buf));
    assert(resp_len > 0);

    /* 8. Launch Application via Host Protocol */
    slvr_frame_t launch_cmd;
    memset(&launch_cmd, 0, sizeof(launch_cmd));
    launch_cmd.version = SLVR_VERSION_1;
    launch_cmd.type = SLVR_CMD_LAUNCH_APP;
    launch_cmd.sequence = 3U;
    launch_cmd.length = 1U;
    launch_cmd.payload[0] = (uint8_t)MK_APP_RETRO_GAMES;

    uint8_t launch_raw[SLVR_MAX_FRAME_LEN];
    size_t launch_len = 0U;
    assert(slvr_encode_frame(&launch_cmd, launch_raw, sizeof(launch_raw), &launch_len) == MK_STATUS_OK);

    protocol_service_inject_rx_bytes(launch_raw, launch_len);
    protocol_service_task(NULL);

    mk_kernel_t *k = mk_kernel_get_instance();
    assert(k->active_app == (uint8_t)MK_APP_RETRO_GAMES);

    printf("[PASS] SliverOS Host Protocol (SLVR/1) Unit Tests Passed Successfully.\n");
}
