/**
 * host_protocol.test.js
 * Unit tests for JavaScript SLVR/1 framing, CRC16, and streaming parser.
 */

import test from 'node:test';
import assert from 'node:assert/strict';
import {
  MsgType,
  slvrCrc16,
  encodeFrame,
  SlvrStreamParser,
  SLVR_FRAME_OVERHEAD,
  SLVR_VERSION_1,
} from '../src/protocol/host_protocol.js';

test('CRC16-CCITT Determinism across standard vector', () => {
  const testVec = new TextEncoder().encode('123456789');
  const crc = slvrCrc16(testVec);
  // Standard CCITT CRC-16 (False) over "123456789" is 0x29B1
  assert.equal(crc, 0x29B1);
});

test('SLVR/1 Frame Encoding structure & CRC calculation', () => {
  const payload = new Uint8Array([0x10, 0x20, 0x30, 0x40]);
  const frame = {
    type: MsgType.CMD_GET_INFO,
    flags: 0,
    sequence: 5,
    payload,
  };

  const wire = encodeFrame(frame);
  assert.equal(wire.length, SLVR_FRAME_OVERHEAD + 4);

  // Check Magic
  assert.equal(wire[0], 0x53); // 'S'
  assert.equal(wire[1], 0x4C); // 'L'
  assert.equal(wire[2], 0x56); // 'V'
  assert.equal(wire[3], 0x52); // 'R'

  // Check Version, Type, Flags, Seq, Len
  assert.equal(wire[4], SLVR_VERSION_1);
  assert.equal(wire[5], MsgType.CMD_GET_INFO);
  assert.equal(wire[6], 0);
  assert.equal(wire[7], 5);
  assert.equal(wire[8], 0);
  assert.equal(wire[9], 4);

  // Check Payload
  assert.deepEqual(wire.subarray(10, 14), payload);

  // Verify CRC
  const expectedCrc = slvrCrc16(wire.subarray(4, 14));
  const wireCrc = (wire[14] << 8) | wire[15];
  assert.equal(wireCrc, expectedCrc);
});

test('SlvrStreamParser decode single complete frame', () => {
  let receivedFrame = null;
  const parser = new SlvrStreamParser(f => {
    receivedFrame = f;
  });

  const payload = new TextEncoder().encode('HELLO_SLIVER');
  const wire = encodeFrame({
    type: MsgType.HELLO,
    sequence: 1,
    payload,
  });

  parser.feed(wire);
  assert.ok(receivedFrame);
  assert.equal(receivedFrame.version, SLVR_VERSION_1);
  assert.equal(receivedFrame.type, MsgType.HELLO);
  assert.equal(receivedFrame.sequence, 1);
  assert.equal(new TextDecoder().decode(receivedFrame.payload), 'HELLO_SLIVER');
});

test('SlvrStreamParser rejects frame with corrupted CRC', () => {
  let receivedFrame = null;
  const parser = new SlvrStreamParser(f => {
    receivedFrame = f;
  });

  const wire = encodeFrame({
    type: MsgType.CMD_GET_STATUS,
    sequence: 2,
    payload: new Uint8Array([1, 2, 3]),
  });

  // Corrupt last byte
  wire[wire.length - 1] ^= 0xFF;

  parser.feed(wire);
  assert.equal(receivedFrame, null);
  assert.ok(parser.crcErrors > 0);
});

test('SlvrStreamParser byte-by-byte chunking & noise resynchronization', () => {
  let received = [];
  const parser = new SlvrStreamParser(f => {
    received.push(f);
  });

  const f1 = encodeFrame({ type: MsgType.CMD_CONNECT, sequence: 10 });
  const f2 = encodeFrame({ type: MsgType.CMD_PING, sequence: 11 });

  // Stream: 15 bytes garbage + f1 + 8 bytes garbage + f2
  const stream = new Uint8Array(15 + f1.length + 8 + f2.length);
  stream.fill(0xEE, 0, 15);
  stream.set(f1, 15);
  stream.fill(0xDD, 15 + f1.length, 15 + f1.length + 8);
  stream.set(f2, 15 + f1.length + 8);

  // Feed byte-by-byte
  for (let i = 0; i < stream.length; i++) {
    parser.feed(stream.subarray(i, i + 1));
  }

  assert.equal(received.length, 2);
  assert.equal(received[0].type, MsgType.CMD_CONNECT);
  assert.equal(received[0].sequence, 10);
  assert.equal(received[1].type, MsgType.CMD_PING);
  assert.equal(received[1].sequence, 11);
});
