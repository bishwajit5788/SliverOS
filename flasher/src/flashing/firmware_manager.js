/**
 * firmware_manager.js
 * Downloads and pre-verifies binary firmware assets before flashing.
 * Strictly enforces Test Fixture Guards to prevent flashing mock binaries to real hardware.
 */

import { FirmwareVerifier } from './verification.js';

export class FirmwareManager {
  constructor(baseFirmwareUrl = './firmware/', logger = console.log, isProductionMode = true) {
    this.baseUrl = baseFirmwareUrl;
    this.log = logger;
    this.isProductionMode = isProductionMode;
  }

  async fetchAndPrepareImages(targetConfig) {
    if (!targetConfig || !Array.isArray(targetConfig.images)) {
      throw new Error('Target configuration has no images defined.');
    }

    const prepared = [];

    for (const img of targetConfig.images) {
      const url = `${this.baseUrl}${img.filename}`;
      this.log(`[INFO] Fetching firmware asset: ${img.filename}...`);

      const resp = await fetch(url);
      if (!resp.ok) {
        throw new Error(`Failed to fetch ${img.filename}: HTTP ${resp.status}`);
      }

      const buffer = await resp.arrayBuffer();
      const uint8Data = new Uint8Array(buffer);

      /* Test Fixture Guard: Reject mock fixtures from production flashing */
      if (this.isProductionMode) {
        const headerSlice = new TextDecoder('utf-8', { fatal: false }).decode(uint8Data.slice(0, 64));
        if (img.is_fixture === true || headerSlice.includes('TEST FIXTURE') || headerSlice.includes('NOT FOR HARDWARE')) {
          throw new Error(
            `[SECURITY REJECTION] Image '${img.filename}' is labeled as a TEST FIXTURE (NOT FOR HARDWARE FLASHING). ` +
            `Production flashing mode prohibits writing test fixtures to real hardware. ` +
            `Please flash binaries produced by 'idf.py build'.`
          );
        }
      }

      this.log(`[INFO] Verifying SHA-256 integrity for ${img.name}...`);
      const verification = await FirmwareVerifier.verifyIntegrity(uint8Data, img.sha256);

      if (!verification.valid) {
        throw new Error(
          `Integrity check failed for ${img.filename}! Expected ${img.sha256}, got ${verification.computed}`
        );
      }

      const offsetNum = typeof img.offset === 'string' ? parseInt(img.offset, 16) : img.offset;

      prepared.push({
        name: img.name,
        filename: img.filename,
        offset: offsetNum,
        data: uint8Data,
        size: uint8Data.length,
        sha256: verification.computed
      });
    }

    return prepared;
  }
}
