/**
 * host_protocol.js
 * Browser-side implementation of the SliverOS Host UI Protocol (SLVR/1).
 *
 * Framing Format:
 * [MAGIC: 4B ("SLVR")] [VERSION: 1B (0x01)] [TYPE: 1B] [FLAGS: 1B] [SEQ: 1B] [LEN: 2B (BE)] [PAYLOAD: N bytes] [CRC16: 2B (BE)]
 */

export const SLVR_MAGIC = new Uint8Array([0x53, 0x4C, 0x56, 0x52]); // "SLVR"
export const SLVR_VERSION_1 = 0x01;
export const SLVR_HEADER_LEN = 10;
export const SLVR_CRC_LEN = 2;
export const SLVR_FRAME_OVERHEAD = SLVR_HEADER_LEN + SLVR_CRC_LEN; // 12
export const SLVR_MAX_PAYLOAD_LEN = 512;
export const SLVR_MAX_FRAME_LEN = SLVR_FRAME_OVERHEAD + SLVR_MAX_PAYLOAD_LEN; // 524

export const MsgType = {
  // ESP32 -> Host
  HELLO: 0x01,
  DEVICE_INFO: 0x02,
  READY: 0x03,
  DESKTOP_STATE: 0x04,
  APP_STATE: 0x05,
  DEVICE_STATUS: 0x06,
  MEMORY_STATUS: 0x07,
  STORAGE_STATUS: 0x08,
  LOG: 0x09,
  ERROR: 0x0A,
  EVENT: 0x0B,
  PONG: 0x0C,
  GAME_STATE: 0x0D,
  TERMINAL_OUTPUT: 0x0E,

  // Host -> ESP32 Commands
  CMD_CONNECT: 0x81,
  CMD_GET_INFO: 0x82,
  CMD_GET_STATUS: 0x83,
  CMD_LAUNCH_APP: 0x84,
  CMD_EXIT_APP: 0x85,
  CMD_APP_INPUT: 0x86,
  CMD_KEY_EVENT: 0x87,
  CMD_BUTTON_EVENT: 0x88,
  CMD_NAVIGATION: 0x89,
  CMD_TERMINAL_INPUT: 0x8A,
  CMD_GET_LOGS: 0x8B,
  CMD_RESET: 0x8C,
  CMD_PING: 0x8D,
};

/**
 * Standard CRC-16-CCITT (Polynomial 0x1021, Initial value 0xFFFF)
 * @param {Uint8Array} data
 * @returns {number} 16-bit unsigned CRC
 */
export function slvrCrc16(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] << 8) & 0xFFFF;
    for (let bit = 0; bit < 8; bit++) {
      if ((crc & 0x8000) !== 0) {
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
  }
  return crc;
}

/**
 * Encodes a SLVR/1 frame into wire bytes
 * @param {Object} frame { type, flags, sequence, payload: Uint8Array|null }
 * @returns {Uint8Array} Encoded frame
 */
export function encodeFrame(frame) {
  const payload = frame.payload ? new Uint8Array(frame.payload) : new Uint8Array(0);
  if (payload.length > SLVR_MAX_PAYLOAD_LEN) {
    throw new Error(`Payload exceeds maximum length of ${SLVR_MAX_PAYLOAD_LEN}`);
  }

  const totalLen = SLVR_FRAME_OVERHEAD + payload.length;
  const out = new Uint8Array(totalLen);

  // 1. Magic
  out.set(SLVR_MAGIC, 0);

  // 2. Header
  out[4] = SLVR_VERSION_1;
  out[5] = frame.type;
  out[6] = frame.flags || 0;
  out[7] = (frame.sequence || 0) & 0xFF;
  out[8] = (payload.length >> 8) & 0xFF;
  out[9] = payload.length & 0xFF;

  // 3. Payload
  if (payload.length > 0) {
    out.set(payload, 10);
  }

  // 4. CRC-16 over bytes 4 through 9 + payload
  const crcData = out.subarray(4, 10 + payload.length);
  const crc = slvrCrc16(crcData);

  out[10 + payload.length] = (crc >> 8) & 0xFF;
  out[11 + payload.length] = crc & 0xFF;

  return out;
}

/**
 * Streaming parser for SLVR/1 binary frames
 */
export class SlvrStreamParser {
  constructor(onFrameCallback) {
    this.onFrame = onFrameCallback || (() => {});
    this.buffer = new Uint8Array(SLVR_MAX_FRAME_LEN * 4);
    this.bufLen = 0;
    this.droppedBytes = 0;
    this.crcErrors = 0;
  }

  reset() {
    this.bufLen = 0;
  }

  /**
   * Push incoming chunk from Web Serial or simulator
   * @param {Uint8Array} chunk 
   */
  feed(chunk) {
    if (!chunk || chunk.length === 0) return;

    // Expand buffer if needed
    if (this.bufLen + chunk.length > this.buffer.length) {
      const nextBuf = new Uint8Array(Math.max(this.buffer.length * 2, this.bufLen + chunk.length + 1024));
      nextBuf.set(this.buffer.subarray(0, this.bufLen), 0);
      this.buffer = nextBuf;
    }

    this.buffer.set(chunk, this.bufLen);
    this.bufLen += chunk.length;

    // Scan for frames
    while (this.bufLen >= SLVR_FRAME_OVERHEAD) {
      // Find magic
      let magicIdx = -1;
      for (let i = 0; i <= this.bufLen - 4; i++) {
        if (
          this.buffer[i] === SLVR_MAGIC[0] &&
          this.buffer[i + 1] === SLVR_MAGIC[1] &&
          this.buffer[i + 2] === SLVR_MAGIC[2] &&
          this.buffer[i + 3] === SLVR_MAGIC[3]
        ) {
          magicIdx = i;
          break;
        }
      }

      if (magicIdx === -1) {
        // Keep last 3 bytes in case magic is split across chunks
        const keep = Math.min(3, this.bufLen);
        this.droppedBytes += (this.bufLen - keep);
        this.buffer.copyWithin(0, this.bufLen - keep, this.bufLen);
        this.bufLen = keep;
        break;
      }

      if (magicIdx > 0) {
        // Discard garbage before magic
        this.droppedBytes += magicIdx;
        this.buffer.copyWithin(0, magicIdx, this.bufLen);
        this.bufLen -= magicIdx;
      }

      // Check if we have at least header length
      if (this.bufLen < SLVR_HEADER_LEN) {
        break;
      }

      const version = this.buffer[4];
      if (version !== SLVR_VERSION_1) {
        // Incompatible version, discard magic byte to re-sync
        this.droppedBytes++;
        this.buffer.copyWithin(0, 1, this.bufLen);
        this.bufLen--;
        continue;
      }

      const type = this.buffer[5];
      const flags = this.buffer[6];
      const sequence = this.buffer[7];
      const payloadLen = (this.buffer[8] << 8) | this.buffer[9];

      if (payloadLen > SLVR_MAX_PAYLOAD_LEN) {
        // Malformed payload length, discard magic byte to re-sync
        this.droppedBytes++;
        this.buffer.copyWithin(0, 1, this.bufLen);
        this.bufLen--;
        continue;
      }

      const totalFrameLen = SLVR_FRAME_OVERHEAD + payloadLen;
      if (this.bufLen < totalFrameLen) {
        // Waiting for more chunk data
        break;
      }

      // We have a complete frame candidate; verify CRC
      const crcExpected = (this.buffer[10 + payloadLen] << 8) | this.buffer[11 + payloadLen];
      const crcActual = slvrCrc16(this.buffer.subarray(4, 10 + payloadLen));

      if (crcExpected === crcActual) {
        const payload = this.buffer.slice(10, 10 + payloadLen);
        const frame = {
          version,
          type,
          flags,
          sequence,
          length: payloadLen,
          payload,
          crc: crcActual,
        };

        // Advance buffer past frame
        this.buffer.copyWithin(0, totalFrameLen, this.bufLen);
        this.bufLen -= totalFrameLen;

        this.onFrame(frame);
      } else {
        // CRC mismatch: discard magic byte and hunt for next
        this.crcErrors++;
        this.droppedBytes++;
        this.buffer.copyWithin(0, 1, this.bufLen);
        this.bufLen--;
      }
    }
  }
}

/**
 * Payload Unpackers
 */
export function unpackDeviceInfo(payload) {
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const decoder = new TextDecoder('utf-8');

  const readString = (offset, maxLen) => {
    let len = 0;
    while (len < maxLen && payload[offset + len] !== 0) len++;
    return decoder.decode(payload.subarray(offset, offset + len));
  };

  return {
    product: readString(0, 24),
    version: readString(24, 12),
    buildId: readString(36, 24),
    gitRevision: readString(60, 24),
    chipFamily: readString(84, 16),
    chipRevision: view.getUint32(100, true),
    flashSizeBytes: view.getUint32(104, true),
    psramSizeBytes: view.getUint32(108, true),
    internalSramTotal: view.getUint32(112, true),
    internalSramFree: view.getUint32(116, true),
    psramTotal: view.getUint32(120, true),
    psramFree: view.getUint32(124, true),
  };
}

export function unpackDeviceStatus(payload) {
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const decoder = new TextDecoder('utf-8');

  const tick = view.getUint32(0, true);
  const uptimeSeconds = view.getUint32(4, true);
  const schedulerIterations = view.getUint32(8, true);
  const activeApp = view.getUint8(12);
  const kernelState = view.getUint8(13);
  const taskCount = view.getUint8(14);
  const faultCount = view.getUint8(15);

  const tasks = [];
  let offset = 16;
  for (let i = 0; i < 8; i++) {
    if (offset + 32 <= payload.length) {
      const id = view.getUint8(offset);
      const state = view.getUint8(offset + 1);
      const priority = view.getUint8(offset + 2);
      const periodTicks = view.getUint32(offset + 4, true);
      const executionCount = view.getUint32(offset + 8, true);
      const lastExecutionUs = view.getUint32(offset + 12, true);
      const worstExecutionUs = view.getUint32(offset + 16, true);
      const overrunCount = view.getUint32(offset + 20, true);

      let nameLen = 0;
      while (nameLen < 12 && payload[offset + 24 + nameLen] !== 0) nameLen++;
      const name = decoder.decode(payload.subarray(offset + 24, offset + 24 + nameLen));

      tasks.push({
        id,
        state,
        priority,
        periodTicks,
        executionCount,
        lastExecutionUs,
        worstExecutionUs,
        overrunCount,
        name,
      });
      offset += 36;
    }
  }

  return {
    tick,
    uptimeSeconds,
    schedulerIterations,
    activeApp,
    kernelState,
    taskCount,
    faultCount,
    tasks,
  };
}

export function unpackMemoryStatus(payload) {
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    arenaCapacity: view.getUint32(0, true),
    arenaUsed: view.getUint32(4, true),
    arenaFree: view.getUint32(8, true),
    arenaPeak: view.getUint32(12, true),
    allocationCount: view.getUint32(16, true),
    freeCount: view.getUint32(20, true),
    failedAllocations: view.getUint32(24, true),
    psramTotal: view.getUint32(28, true),
    psramFree: view.getUint32(32, true),
  };
}

export function unpackStorageStatus(payload) {
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    totalSectors: view.getUint32(0, true),
    freeSectors: view.getUint32(4, true),
    activeSectors: view.getUint32(8, true),
    obsoleteSectors: view.getUint32(12, true),
    recordCommitCount: view.getUint32(16, true),
  };
}

export function unpackGameState(payload) {
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    x: view.getInt16(0, true) / 10,
    y: view.getInt16(2, true) / 10,
    vx: view.getInt16(4, true),
    vy: view.getInt16(6, true),
    fuel: view.getUint16(8, true),
    score: view.getUint16(10, true),
    status: view.getUint8(12), // 0: PLAYING, 1: LANDED, 2: CRASHED
    padX: view.getUint8(13),
    padY: view.getUint8(14),
    padW: view.getUint8(15),
    frameSeq: view.getUint32(16, true),
  };
}

export function unpackLog(payload) {
  const level = payload[0];
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const timestampMs = view.getUint32(1, true);
  let textLen = 0;
  while (textLen < 120 && payload[5 + textLen] !== 0) textLen++;
  const text = new TextDecoder('utf-8').decode(payload.subarray(5, 5 + textLen));
  return { level, timestampMs, text };
}

export function unpackTerminal(payload) {
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const textLen = view.getUint16(0, true);
  const text = new TextDecoder('utf-8').decode(payload.subarray(2, 2 + textLen));
  return { text };
}
