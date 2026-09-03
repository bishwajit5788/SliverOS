/**
 * protocol.test.js
 * Unit tests for SLIP encoding/decoding, checksum calculation, and packet builders.
 */

import test from 'node:test';
import assert from 'node:assert/strict';
import {
  slipEncode,
  slipDecode,
  calcChecksum,
  formatCommandPacket,
  parseResponsePacket,
  SLIP_END,
  SLIP_ESC,
  SLIP_ESC_END,
  SLIP_ESC_ESC,
  ESP_CMD_SYNC
} from '../src/serial/esp_protocol.js';

test('SLIP Framing - basic bytes without escape', () => {
  const input = new Uint8Array([0x01, 0x02, 0x03, 0x04]);
  const encoded = slipEncode(input);

  assert.equal(encoded[0], SLIP_END);
  assert.equal(encoded[encoded.length - 1], SLIP_END);
  assert.deepEqual(encoded.subarray(1, encoded.length - 1), input);

  const decoded = slipDecode(encoded);
  assert.deepEqual(decoded, input);
});

test('SLIP Framing - escape 0xC0 and 0xDB', () => {
  const input = new Uint8Array([0xC0, 0xDB, 0x55]);
  const encoded = slipEncode(input);

  // 0xC0 -> 0xDB 0xDC
  // 0xDB -> 0xDB 0xDD
  const expected = new Uint8Array([
    SLIP_END,
    SLIP_ESC, SLIP_ESC_END,
    SLIP_ESC, SLIP_ESC_ESC,
    0x55,
    SLIP_END
  ]);
  assert.deepEqual(encoded, expected);

  const decoded = slipDecode(encoded);
  assert.deepEqual(decoded, input);
});

test('XOR Checksum Calculation', () => {
  // Test with seed 0xEF
  const data = new Uint8Array([0x10, 0x20, 0x30]);
  const expected = 0xEF ^ 0x10 ^ 0x20 ^ 0x30;
  assert.equal(calcChecksum(data), expected);
});

test('Command Packet Construction & Parsing', () => {
  const payload = new Uint8Array([0xAA, 0xBB, 0xCC, 0xDD]);
  const packet = formatCommandPacket(ESP_CMD_SYNC, payload, 0x12345678);

  // Raw mock response packet:
  // Direction: 0x01 (from ESP)
  // Opcode: ESP_CMD_SYNC (0x08)
  // Size: 2 (0x02, 0x00)
  // Value: 0x00000000
  // Data: 0x55, 0x66
  // Status: 0x00, Error: 0x00
  const mockResp = new Uint8Array([
    SLIP_END,
    0x01, ESP_CMD_SYNC, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x55, 0x66,
    0x00, 0x00,
    SLIP_END
  ]);

  const parsed = parseResponsePacket(mockResp);
  assert.equal(parsed.direction, 0x01);
  assert.equal(parsed.opcode, ESP_CMD_SYNC);
  assert.equal(parsed.size, 2);
  assert.equal(parsed.status, 0);
  assert.equal(parsed.error, 0);
  assert.equal(parsed.success, true);
  assert.deepEqual(parsed.payload, new Uint8Array([0x55, 0x66]));
});
