/**
 * device_detector.js
 * Uses Espressif's maintained esptool-js ROM implementation for reliable
 * Web Serial synchronization and chip detection.
 */

import { ESPLoader, Transport } from 'esptool-js';
import { DeviceInfo } from './device_info.js';

export class DeviceDetector {
  constructor(serialPort, logger) {
    this.serial = serialPort;
    this.log = logger;
    this.transport = null;
    this.loader = null;
  }

  async connectAndDetect(baudRate = 115200) {
    if (!this.serial.port) {
      throw new Error('No USB serial device selected. Click Connect and choose the ESP32-S3 port.');
    }

    this.log(`[INFO] Connecting to ESP32 ROM bootloader at ${baudRate} baud...`);
    this.transport = new Transport(this.serial.port, false);

    const terminal = {
      clean: () => {},
      writeLine: (data) => this.log(`[ROM] ${data}`),
      write: (data) => this.log(`[ROM] ${data}`)
    };

    this.loader = new ESPLoader({
      transport: this.transport,
      baudrate: baudRate,
      terminal,
      debugLogging: false
    });

    let chipName;
    try {
      chipName = await this.loader.main();
    } catch (error) {
      throw new Error(
        `ESP32 bootloader connection failed. Put the board in download mode (hold BOOT, tap RESET, release BOOT) and try again. ${error.message}`
      );
    }

    const normalized = String(chipName).toUpperCase();
    let targetKey = 'esp32';
    if (normalized.includes('ESP32-S3')) targetKey = 'esp32s3';
    else if (normalized.includes('ESP32-C3')) targetKey = 'esp32c3';
    else if (normalized.includes('ESP32')) targetKey = 'esp32';

    if (targetKey !== 'esp32s3') {
      throw new Error(`Unsupported chip detected: ${chipName}. SliverOS currently requires ESP32-S3.`);
    }

    const chip = {
      family: 'ESP32-S3',
      targetKey: 'esp32s3',
      revision: this.loader.chip?.REVISION ?? 0,
      flashSize: 'detect'
    };

    try {
      const detectedSize = await this.loader.detectFlashSize();
      if (detectedSize) chip.flashSize = detectedSize;
    } catch (error) {
      this.log(`[WARN] Flash-size detection unavailable: ${error.message}`);
    }

    return {
      chip,
      loader: this.loader,
      transport: this.transport,
      displayName: DeviceInfo.getChipDisplayName('esp32s3')
    };
  }
}
