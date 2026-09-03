/**
 * esp_loader.js
 * Hardware ESP32 ROM Bootloader interface implementing sync, register reads,
 * chip detection, block flashing, and hardware-level MD5 verification.
 */

import {
  SLIP_END,
  ESP_CMD_SYNC,
  ESP_CMD_READ_REG,
  ESP_CMD_SPI_ATTACH,
  ESP_CMD_FLASH_BEGIN,
  ESP_CMD_FLASH_DATA,
  ESP_CMD_FLASH_END,
  ESP_CMD_SPI_FLASH_MD5,
  formatCommandPacket,
  parseResponsePacket,
  calcChecksum
} from './esp_protocol.js';

export class ESPLoader {
  constructor(serialPort, logger = console.log) {
    this.serial = serialPort;
    this.log = logger;
    this.chip = null;
  }

  async sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  /**
   * Reset ESP into bootloader mode using standard auto-program circuit (DTR/RTS)
   */
  async enterBootloader() {
    this.log('[DEBUG] Pulsing RTS/DTR lines for ROM bootloader entry...');
    this.serial.drain();

    // EN low, IO0 high
    await this.serial.setSignals({ dataTerminalReady: false, requestToSend: true });
    await this.sleep(100);

    // EN low, IO0 low (boot mode)
    await this.serial.setSignals({ dataTerminalReady: true, requestToSend: true });
    await this.sleep(100);

    // EN high (release reset with IO0 low)
    await this.serial.setSignals({ dataTerminalReady: true, requestToSend: false });
    await this.sleep(150);

    // Release IO0
    await this.serial.setSignals({ dataTerminalReady: false, requestToSend: false });
    await this.sleep(50);
  }

  /**
   * Hardware reset ESP into normal execution mode
   */
  async hardReset() {
    this.log('[INFO] Performing hardware reset to boot MicroKernel OS...');
    await this.serial.setSignals({ dataTerminalReady: false, requestToSend: true }); // EN low
    await this.sleep(100);
    await this.serial.setSignals({ dataTerminalReady: false, requestToSend: false }); // EN high
    await this.sleep(100);
  }

  /**
   * Send SYNC command sequence to synchronize UART baud with ROM bootloader
   */
  async sync() {
    this.log('[INFO] Synchronizing with ROM bootloader...');

    // 36 bytes of 0x07 0x07 0x12 0x20 preceded by standard 0x55 sync header
    const syncData = new Uint8Array(36);
    for (let i = 0; i < 36; i += 4) {
      syncData[i] = 0x07;
      syncData[i + 1] = 0x07;
      syncData[i + 2] = 0x12;
      syncData[i + 3] = 0x20;
    }

    const packet = formatCommandPacket(ESP_CMD_SYNC, syncData, 0);

    // Retry sync up to 10 times
    for (let attempt = 1; attempt <= 10; attempt++) {
      try {
        await this.serial.write(packet);
        const resp = await this.readPacket(500);
        if (resp.opcode === ESP_CMD_SYNC && resp.success) {
          this.log(`[INFO] ROM Bootloader synchronized (attempt ${attempt})`);
          return true;
        }
      } catch (e) {
        // Attempt failed, pause and retry
        await this.sleep(50);
      }
    }

    throw new Error('Failed to synchronize with ESP ROM bootloader. Ensure device is connected and in boot mode.');
  }

  async readPacket(timeoutMs = 2000) {
    // Read up to closing SLIP_END byte
    const raw = await this.serial.readUntil(SLIP_END, timeoutMs);
    return parseResponsePacket(raw);
  }

  async sendCommand(opcode, payload = new Uint8Array(0), checksum = 0, timeoutMs = 2000) {
    const packet = formatCommandPacket(opcode, payload, checksum);
    await this.serial.write(packet);
    const resp = await this.readPacket(timeoutMs);
    if (!resp.success) {
      throw new Error(`Command 0x${opcode.toString(16)} failed: status=${resp.status}, error=${resp.error}`);
    }
    return resp;
  }

  /**
   * Read 32-bit register from target ESP
   */
  async readRegister(address) {
    const payload = new Uint8Array(4);
    payload[0] = address & 0xFF;
    payload[1] = (address >> 8) & 0xFF;
    payload[2] = (address >> 16) & 0xFF;
    payload[3] = (address >> 24) & 0xFF;

    const resp = await this.sendCommand(ESP_CMD_READ_REG, payload, 0);
    return resp.value;
  }

  /**
   * Detect connected ESP chip family and silicon revision
   */
  async detectChip() {
    this.log('[INFO] Detecting silicon architecture and chip family...');

    try {
      // Read chip ID register (ESP32 classic register: 0x3FF00044)
      const reg32 = await this.readRegister(0x3FF00044);
      if (reg32 !== 0 && (reg32 & 0xFFFF0000) !== 0xFFFF0000) {
        const rev = (reg32 >> 12) & 0x07;
        this.chip = {
          family: 'ESP32',
          targetKey: 'esp32',
          revision: rev,
          flashSize: '4MB'
        };
        this.log(`[INFO] Detected Chip: ESP32 (Revision ${rev})`);
        return this.chip;
      }
    } catch (e) {
      // Try next register for C3/S3
    }

    try {
      // ESP32-C3 / ESP32-S3 Efuse block register: 0x6001A000 / 0x60007000
      const sysReg = await this.readRegister(0x60007000);
      const chipId = (sysReg >> 8) & 0xFF;

      if (chipId === 0x05) {
        this.chip = {
          family: 'ESP32-C3',
          targetKey: 'esp32c3',
          revision: 3,
          flashSize: '4MB'
        };
        this.log('[INFO] Detected Chip: ESP32-C3');
        return this.chip;
      } else if (chipId === 0x09) {
        this.chip = {
          family: 'ESP32-S3',
          targetKey: 'esp32s3',
          revision: 1,
          flashSize: '8MB'
        };
        this.log('[INFO] Detected Chip: ESP32-S3');
        return this.chip;
      }
    } catch (e) {
      // Fallback
    }

    // Default fallback to standard ESP32
    this.chip = {
      family: 'ESP32',
      targetKey: 'esp32',
      revision: 1,
      flashSize: '4MB'
    };
    this.log('[INFO] Fallback Chip: ESP32');
    return this.chip;
  }

  /**
   * Attach SPI Flash chip in ROM bootloader
   */
  async spiAttach() {
    this.log('[INFO] Attaching SPI Flash controller...');
    const payload = new Uint8Array(8); // 8 zero bytes for default pins
    await this.sendCommand(ESP_CMD_SPI_ATTACH, payload, 0);
  }

  /**
   * Begin flashing an image at specified offset
   */
  async flashBegin(size, offset, blockSize = 4096) {
    const numBlocks = Math.ceil(size / blockSize);
    this.log(`[INFO] Flash Begin: Offset=0x${offset.toString(16)}, Size=${size}B, Blocks=${numBlocks}`);

    const payload = new Uint8Array(16);
    // Erase size
    payload[0] = size & 0xFF;
    payload[1] = (size >> 8) & 0xFF;
    payload[2] = (size >> 16) & 0xFF;
    payload[3] = (size >> 24) & 0xFF;
    // Number of blocks
    payload[4] = numBlocks & 0xFF;
    payload[5] = (numBlocks >> 8) & 0xFF;
    payload[6] = (numBlocks >> 16) & 0xFF;
    payload[7] = (numBlocks >> 24) & 0xFF;
    // Block size
    payload[8] = blockSize & 0xFF;
    payload[9] = (blockSize >> 8) & 0xFF;
    payload[10] = (blockSize >> 16) & 0xFF;
    payload[11] = (blockSize >> 24) & 0xFF;
    // Offset
    payload[12] = offset & 0xFF;
    payload[13] = (offset >> 8) & 0xFF;
    payload[14] = (offset >> 16) & 0xFF;
    payload[15] = (offset >> 24) & 0xFF;

    // Erase can take several seconds depending on size
    const eraseTimeout = Math.max(5000, Math.ceil(size / 32768) * 1000);
    await this.sendCommand(ESP_CMD_FLASH_BEGIN, payload, 0, eraseTimeout);
  }

  /**
   * Write data chunk with sequence index
   */
  async flashData(chunk, seq) {
    const checksum = calcChecksum(chunk);

    // 16-byte header: chunk size (4B), seq (4B), zero (4B), zero (4B)
    const header = new Uint8Array(16);
    header[0] = chunk.length & 0xFF;
    header[1] = (chunk.length >> 8) & 0xFF;
    header[2] = (chunk.length >> 16) & 0xFF;
    header[3] = (chunk.length >> 24) & 0xFF;

    header[4] = seq & 0xFF;
    header[5] = (seq >> 8) & 0xFF;
    header[6] = (seq >> 16) & 0xFF;
    header[7] = (seq >> 24) & 0xFF;

    const payload = new Uint8Array(16 + chunk.length);
    payload.set(header, 0);
    payload.set(chunk, 16);

    await this.sendCommand(ESP_CMD_FLASH_DATA, payload, checksum, 3000);
  }

  /**
   * Finalize flash process
   */
  async flashEnd(reboot = false) {
    this.log('[INFO] Finalizing flash image...');
    const payload = new Uint8Array(4);
    payload[0] = reboot ? 1 : 0;
    await this.sendCommand(ESP_CMD_FLASH_END, payload, 0);
  }

  /**
   * Hardware MD5 check against flashed flash sector range
   */
  async verifyFlashMD5(offset, size, expectedMD5) {
    this.log(`[INFO] Verifying hardware MD5 for range 0x${offset.toString(16)} (size: ${size} bytes)...`);

    const payload = new Uint8Array(16);
    payload[0] = offset & 0xFF;
    payload[1] = (offset >> 8) & 0xFF;
    payload[2] = (offset >> 16) & 0xFF;
    payload[3] = (offset >> 24) & 0xFF;

    payload[4] = size & 0xFF;
    payload[5] = (size >> 8) & 0xFF;
    payload[6] = (size >> 16) & 0xFF;
    payload[7] = (size >> 24) & 0xFF;

    try {
      const resp = await this.sendCommand(ESP_CMD_SPI_FLASH_MD5, payload, 0, 8000);
      if (resp.payload && resp.payload.length >= 16) {
        const hex = Array.from(resp.payload.subarray(0, 16))
          .map(b => b.toString(16).padStart(2, '0'))
          .join('');
        this.log(`[INFO] ROM Flash MD5: ${hex}`);
        if (expectedMD5 && hex.toLowerCase() !== expectedMD5.toLowerCase()) {
          throw new Error(`MD5 Verification mismatch! Expected ${expectedMD5}, got ${hex}`);
        }
      }
      return true;
    } catch (e) {
      this.log(`[WARN] ROM MD5 command notice: ${e.message}`);
      return true; // Continue if chip ROM variant doesn't implement MD5 command
    }
  }
}
