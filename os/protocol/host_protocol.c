/**
 * @file host_protocol.c
 * @brief SliverOS Host Protocol (SLVR/1) Framing, Encoding, and Decoding.
 */

#include "host_protocol.h"
#include <string.h>

/* Standard CRC16-CCITT (Polynomial 0x1021, Initial value 0xFFFF) */
uint16_t slvr_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFU;
    if (data == NULL) {
        return crc;
    }

    for (size_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000U) {
                crc = (crc << 1) ^ 0x1021U;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}

void slvr_parser_init(slvr_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }

    memset(parser, 0, sizeof(slvr_parser_t));
    parser->state = SLVR_PARSE_STATE_MAGIC_0;
}

bool slvr_parser_feed_byte(slvr_parser_t *parser, uint8_t byte, slvr_frame_t *out_frame)
{
    if (parser == NULL || out_frame == NULL) {
        return false;
    }

    switch (parser->state) {
        case SLVR_PARSE_STATE_MAGIC_0:
            if (byte == SLVR_MAGIC_0) {
                parser->state = SLVR_PARSE_STATE_MAGIC_1;
            } else {
                parser->dropped_bytes++;
            }
            break;

        case SLVR_PARSE_STATE_MAGIC_1:
            if (byte == SLVR_MAGIC_1) {
                parser->state = SLVR_PARSE_STATE_MAGIC_2;
            } else if (byte == SLVR_MAGIC_0) {
                parser->state = SLVR_PARSE_STATE_MAGIC_1;
                parser->dropped_bytes++;
            } else {
                parser->state = SLVR_PARSE_STATE_MAGIC_0;
                parser->dropped_bytes += 2U;
            }
            break;

        case SLVR_PARSE_STATE_MAGIC_2:
            if (byte == SLVR_MAGIC_2) {
                parser->state = SLVR_PARSE_STATE_MAGIC_3;
            } else {
                parser->state = SLVR_PARSE_STATE_MAGIC_0;
                parser->dropped_bytes += 3U;
            }
            break;

        case SLVR_PARSE_STATE_MAGIC_3:
            if (byte == SLVR_MAGIC_3) {
                parser->state = SLVR_PARSE_STATE_VERSION;
            } else {
                parser->state = SLVR_PARSE_STATE_MAGIC_0;
                parser->dropped_bytes += 4U;
            }
            break;

        case SLVR_PARSE_STATE_VERSION:
            if (byte == SLVR_VERSION_1) {
                parser->current_frame.version = byte;
                parser->state = SLVR_PARSE_STATE_TYPE;
            } else {
                /* Incompatible version: drop and restart */
                parser->state = SLVR_PARSE_STATE_MAGIC_0;
                parser->dropped_bytes++;
            }
            break;

        case SLVR_PARSE_STATE_TYPE:
            parser->current_frame.type = byte;
            parser->state = SLVR_PARSE_STATE_FLAGS;
            break;

        case SLVR_PARSE_STATE_FLAGS:
            parser->current_frame.flags = byte;
            parser->state = SLVR_PARSE_STATE_SEQ;
            break;

        case SLVR_PARSE_STATE_SEQ:
            parser->current_frame.sequence = byte;
            parser->state = SLVR_PARSE_STATE_LEN_H;
            break;

        case SLVR_PARSE_STATE_LEN_H:
            parser->current_frame.length = ((uint16_t)byte) << 8;
            parser->state = SLVR_PARSE_STATE_LEN_L;
            break;

        case SLVR_PARSE_STATE_LEN_L:
            parser->current_frame.length |= (uint16_t)byte;
            if (parser->current_frame.length > SLVR_MAX_PAYLOAD_LEN) {
                /* Length exceeds safe buffer boundary: reject malformed frame */
                parser->state = SLVR_PARSE_STATE_MAGIC_0;
                parser->dropped_bytes++;
                break;
            }
            parser->payload_idx = 0U;
            if (parser->current_frame.length == 0U) {
                parser->state = SLVR_PARSE_STATE_CRC_H;
            } else {
                parser->state = SLVR_PARSE_STATE_PAYLOAD;
            }
            break;

        case SLVR_PARSE_STATE_PAYLOAD:
            parser->current_frame.payload[parser->payload_idx++] = byte;
            if (parser->payload_idx >= parser->current_frame.length) {
                parser->state = SLVR_PARSE_STATE_CRC_H;
            }
            break;

        case SLVR_PARSE_STATE_CRC_H:
            parser->current_frame.crc16 = ((uint16_t)byte) << 8;
            parser->state = SLVR_PARSE_STATE_CRC_L;
            break;

        case SLVR_PARSE_STATE_CRC_L: {
            parser->current_frame.crc16 |= (uint16_t)byte;
            parser->state = SLVR_PARSE_STATE_MAGIC_0;

            /* Compute expected CRC over: Version, Type, Flags, Seq, Len_H, Len_L, Payload */
            uint8_t header_bytes[6];
            header_bytes[0] = parser->current_frame.version;
            header_bytes[1] = parser->current_frame.type;
            header_bytes[2] = parser->current_frame.flags;
            header_bytes[3] = parser->current_frame.sequence;
            header_bytes[4] = (uint8_t)(parser->current_frame.length >> 8);
            header_bytes[5] = (uint8_t)(parser->current_frame.length & 0xFFU);

            uint16_t expected_crc = slvr_crc16(header_bytes, sizeof(header_bytes));
            if (parser->current_frame.length > 0U) {
                /* Continue CRC calculation over payload */
                uint16_t crc_intermediate = expected_crc;
                for (size_t i = 0; i < parser->current_frame.length; i++) {
                    crc_intermediate ^= ((uint16_t)parser->current_frame.payload[i]) << 8;
                    for (uint8_t bit = 0; bit < 8; bit++) {
                        if (crc_intermediate & 0x8000U) {
                            crc_intermediate = (crc_intermediate << 1) ^ 0x1021U;
                        } else {
                            crc_intermediate = (crc_intermediate << 1);
                        }
                    }
                }
                expected_crc = crc_intermediate;
            }

            if (parser->current_frame.crc16 == expected_crc) {
                *out_frame = parser->current_frame;
                parser->frames_received++;
                return true;
            } else {
                parser->crc_errors++;
            }
            break;
        }

        default:
            parser->state = SLVR_PARSE_STATE_MAGIC_0;
            break;
    }

    return false;
}

mk_status_t slvr_encode_frame(const slvr_frame_t *frame, uint8_t *out_buf,
                             size_t max_buf_len, size_t *out_frame_len)
{
    if (frame == NULL || out_buf == NULL || out_frame_len == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    if (frame->length > SLVR_MAX_PAYLOAD_LEN) {
        return MK_STATUS_INVALID_ARG;
    }

    size_t total_len = SLVR_FRAME_OVERHEAD + (size_t)frame->length;
    if (max_buf_len < total_len) {
        return MK_STATUS_NO_MEMORY;
    }

    /* 1. Magic */
    out_buf[0] = SLVR_MAGIC_0;
    out_buf[1] = SLVR_MAGIC_1;
    out_buf[2] = SLVR_MAGIC_2;
    out_buf[3] = SLVR_MAGIC_3;

    /* 2. Header */
    out_buf[4] = frame->version;
    out_buf[5] = frame->type;
    out_buf[6] = frame->flags;
    out_buf[7] = frame->sequence;
    out_buf[8] = (uint8_t)(frame->length >> 8);
    out_buf[9] = (uint8_t)(frame->length & 0xFFU);

    /* 3. Payload */
    if (frame->length > 0U) {
        memcpy(&out_buf[10], frame->payload, frame->length);
    }

    /* 4. CRC-16 (calculated over bytes 4 through 9 + payload) */
    uint16_t crc = slvr_crc16(&out_buf[4], 6U + (size_t)frame->length);

    out_buf[10 + frame->length] = (uint8_t)(crc >> 8);
    out_buf[11 + frame->length] = (uint8_t)(crc & 0xFFU);

    *out_frame_len = total_len;
    return MK_STATUS_OK;
}
