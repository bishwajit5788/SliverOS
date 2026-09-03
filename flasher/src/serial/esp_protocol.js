/**
 * esp_protocol.js
 * ESP32 ROM Bootloader serial protocol constants, SLIP framing, and packet builders.
 * References: Espressif ROM Bootloader Protocol Specification.
 */

/* SLIP Framing Characters (RFC 1055) */
export const SLIP_END     = 0xC0;
export const SLIP_ESC     = 0xDB;
export const SLIP_ESC_END = 0xDC;
export const SLIP_ESC_ESC = 0xDD;

/* ESP Bootloader Command Opcodes */
export const ESP_CMD_FLASH_BEGIN   = 0x02;
export const ESP_CMD_FLASH_DATA    = 0x03;
export const ESP_CMD_FLASH_END     = 0x04;
export const ESP_CMD_MEM_BEGIN     = 0x05;
export const ESP_CMD_MEM_END       = 0x06;
export const ESP_CMD_MEM_DATA      = 0x07;
export const ESP_CMD_SYNC          = 0x08;
export const ESP_CMD_WRITE_REG     = 0x09;
export const ESP_CMD_READ_REG      = 0x0A;
export const ESP_CMD_SPI_ATTACH    = 0x0D;
export const ESP_CMD_CHANGE_BAUD   = 0x0F;
export const ESP_CMD_SPI_FLASH_MD5 = 0x13;

/* Chip Magic Numbers / Register addresses */
export const ESP32_CHIP_REV_REG    = 0x3FF00044;
export const ESP32C3_CHIP_REV_REG  = 0x60007000;
export const ESP32S3_CHIP_REV_REG  = 0x60007000;

/**
 * Encode a byte buffer with SLIP framing.
 * @param {Uint8Array} data 
 * @returns {Uint8Array} SLIP encoded buffer surrounded by 0xC0
 */
export function slipEncode(data) {
  const result = [];
  result.push(SLIP_END);

  for (let i = 0; i < data.length; i++) {
    const b = data[i];
    if (b === SLIP_END) {
      result.push(SLIP_ESC);
      result.push(SLIP_ESC_END);
    } else if (b === SLIP_ESC) {
      result.push(SLIP_ESC);
      result.push(SLIP_ESC_ESC);
    } else {
      result.push(b);
    }
  }

  result.push(SLIP_END);
  return new Uint8Array(result);
}

/**
 * Decode a SLIP-framed byte buffer.
 * @param {Uint8Array} data 
 * @returns {Uint8Array} Unescaped payload buffer
 */
export function slipDecode(data) {
  const result = [];
  let escaped = false;

  for (let i = 0; i < data.length; i++) {
    const b = data[i];
    if (b === SLIP_END) {
      continue;
    }
    if (escaped) {
      if (b === SLIP_ESC_END) {
        result.push(SLIP_END);
      } else if (b === SLIP_ESC_ESC) {
        result.push(SLIP_ESC);
      } else {
        result.push(b);
      }
      escaped = false;
    } else if (b === SLIP_ESC) {
      escaped = true;
    } else {
      result.push(b);
    }
  }

  return new Uint8Array(result);
}

/**
 * Calculate XOR checksum seeded with 0xEF as per Espressif specification.
 * @param {Uint8Array} data 
 * @returns {number} 32-bit checksum value
 */
export function calcChecksum(data) {
  let checksum = 0xEF;
  for (let i = 0; i < data.length; i++) {
    checksum ^= data[i];
  }
  return checksum >>> 0;
}

/**
 * Format a binary command packet for transmission to ESP bootloader.
 * Header format (8 bytes little-endian):
 * Direction (0x00 for host request) [1 byte]
 * Opcode [1 byte]
 * Data length [2 bytes]
 * Checksum [4 bytes]
 * Followed by Payload.
 * 
 * @param {number} opcode 
 * @param {Uint8Array} payload 
 * @param {number} checksum 
 * @returns {Uint8Array} Formatted SLIP packet ready to send
 */
export function formatCommandPacket(opcode, payload = new Uint8Array(0), checksum = 0) {
  const header = new Uint8Array(8);
  header[0] = 0x00; // Direction: 0 = Host to Bootloader
  header[1] = opcode;
  header[2] = payload.length & 0xFF;
  header[3] = (payload.length >> 8) & 0xFF;

  header[4] = checksum & 0xFF;
  header[5] = (checksum >> 8) & 0xFF;
  header[6] = (checksum >> 16) & 0xFF;
  header[7] = (checksum >> 24) & 0xFF;

  const packet = new Uint8Array(8 + payload.length);
  packet.set(header, 0);
  packet.set(payload, 8);

  return slipEncode(packet);
}

/**
 * Parse an incoming bootloader response packet.
 * Response Header format:
 * Direction (0x01) [1 byte]
 * Opcode [1 byte]
 * Size [2 bytes]
 * Value [4 bytes]
 * Data [N bytes]
 * Status [2 bytes at end: 1 byte status (0 = success), 1 byte error]
 */
export function parseResponsePacket(data) {
  const decoded = slipDecode(data);
  if (decoded.length < 10) {
    throw new Error(`Response too short: ${decoded.length} bytes`);
  }

  const direction = decoded[0];
  const opcode = decoded[1];
  const size = decoded[2] | (decoded[3] << 8);
  const value = (decoded[4] | (decoded[5] << 8) | (decoded[6] << 16) | (decoded[7] << 24)) >>> 0;

  const payload = decoded.subarray(8, 8 + size);
  const statusOffset = decoded.length - 2;
  const status = decoded[statusOffset];
  const error = decoded[statusOffset + 1];

  return {
    direction,
    opcode,
    size,
    value,
    payload,
    status,
    error,
    success: (status === 0 && error === 0)
  };
}
