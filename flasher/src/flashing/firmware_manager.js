/**
 * firmware_manager.js
 * Downloads and pre-verifies immutable firmware assets before flashing.
 */

import { FirmwareVerifier } from './verification.js';

export class FirmwareManager {
  constructor(baseFirmwareUrl = './firmware/', logger = console.log, isProductionMode = true) {
    this.baseUrl = baseFirmwareUrl;
    this.log = logger;
    this.isProductionMode = isProductionMode;
  }

  async fetchAndPrepareImages(targetConfig) {
    if (!targetConfig || !Array.isArray(targetConfig.images) || targetConfig.images.length === 0) {
      throw new Error('No firmware images are published for this target.');
    }

    const prepared = [];
    for (const image of targetConfig.images) {
      const url = image.url || image.filename?.startsWith('http')
        ? (image.url || image.filename)
        : `${this.baseUrl}${image.filename}`;

      if (!url) throw new Error(`Firmware image ${image.name || 'unknown'} has no download URL.`);
      this.log(`[INFO] Fetching verified firmware asset: ${image.name || image.filename}...`);

      const response = await fetch(url, { cache: 'no-store' });
      if (!response.ok) throw new Error(`Failed to fetch firmware asset: HTTP ${response.status}`);

      const data = new Uint8Array(await response.arrayBuffer());

      if (this.isProductionMode) {
        const headerSlice = new TextDecoder('utf-8', { fatal: false }).decode(data.slice(0, 64));
        if (image.is_fixture === true || headerSlice.includes('TEST FIXTURE') || headerSlice.includes('NOT FOR HARDWARE')) {
          throw new Error(`Firmware asset '${image.name}' is marked as a test fixture and cannot be flashed.`);
        }
      }

      if (!image.sha256) throw new Error(`Firmware asset '${image.name}' has no expected SHA-256 digest.`);
      const verification = await FirmwareVerifier.verifyIntegrity(data, image.sha256);
      if (!verification.valid) {
        throw new Error(`SHA-256 verification failed for ${image.name}: expected ${image.sha256}, got ${verification.computed}`);
      }

      const offset = typeof image.offset === 'string' ? Number.parseInt(image.offset, 0) : image.offset;
      if (!Number.isInteger(offset) || offset < 0 || (offset % 0x1000) !== 0 && offset !== 0x8000 && offset !== 0xF000) {
        throw new Error(`Invalid flash offset for ${image.name}: ${image.offset}`);
      }

      prepared.push({
        name: image.name,
        filename: image.filename || image.name,
        offset,
        data,
        size: data.length,
        sha256: verification.computed
      });
    }
    return prepared;
  }
}
