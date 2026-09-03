/**
 * device_detector.js
 * Device detection orchestrator that manages ROM synchronization,
 * registers reading, and chip validation.
 */

import { ESPLoader } from '../serial/esp_loader.js';
import { DeviceInfo } from './device_info.js';

export class DeviceDetector {
  constructor(serialPort, logger) {
    this.serial = serialPort;
    this.log = logger;
    this.loader = new ESPLoader(serialPort, logger);
  }

  async connectAndDetect(baudRate = 115200) {
    this.log('[INFO] Opening Web Serial port at ' + baudRate + ' baud...');
    await this.serial.open(baudRate);

    this.log('[INFO] Initiating ROM Bootloader auto-reset sequence...');
    await this.loader.enterBootloader();

    this.log('[INFO] Synchronizing with target device...');
    await this.loader.sync();

    this.log('[INFO] Reading target chip metadata...');
    const chip = await this.loader.detectChip();

    await this.loader.spiAttach();

    return {
      chip,
      loader: this.loader,
      displayName: DeviceInfo.getChipDisplayName(chip.targetKey)
    };
  }
}
