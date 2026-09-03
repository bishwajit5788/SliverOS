/**
 * manifest.test.js
 * Unit tests for manifest validation, target chip compatibility, and primary target designation.
 */

import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { DeviceInfo } from '../src/device/device_info.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

test('Manifest Schema & Content Validation', () => {
  const manifestPath = path.resolve(__dirname, '../../firmware/manifests/releases.json');
  const raw = fs.readFileSync(manifestPath, 'utf-8');
  const data = JSON.parse(raw);

  assert.equal(data.product, 'MicroKernel OS');
  assert.ok(data.latest);
  assert.ok(Array.isArray(data.releases));
  assert.ok(data.releases.length > 0);

  const release = data.releases[0];
  assert.ok(release.targets.esp32);
  assert.ok(release.targets.esp32c3);
  assert.ok(release.targets.esp32s3);

  // Check structured images for esp32s3
  const esp32s3Images = release.targets.esp32s3.images;
  assert.equal(esp32s3Images.length, 3);
  for (const img of esp32s3Images) {
    assert.ok(img.name);
    assert.ok(img.offset);
    assert.ok(img.filename);
    assert.ok(img.size > 0);
    assert.equal(img.sha256.length, 64); // Valid SHA-256 hex length
  }
});

test('Device Compatibility Matching & Primary Target', () => {
  assert.equal(DeviceInfo.isTargetCompatible('esp32', 'esp32'), true);
  assert.equal(DeviceInfo.isTargetCompatible('ESP32-C3', 'esp32c3'), true);
  assert.equal(DeviceInfo.isTargetCompatible('ESP32-S3', 'esp32s3'), true);

  // Primary release target verification
  assert.equal(DeviceInfo.isPrimaryTarget('esp32s3'), true);
  assert.equal(DeviceInfo.isPrimaryTarget('ESP32-S3'), true);
  assert.equal(DeviceInfo.isPrimaryTarget('esp32'), false);
  assert.equal(DeviceInfo.isPrimaryTarget('esp32c3'), false);

  // Incompatible chip rejection
  assert.equal(DeviceInfo.isTargetCompatible('ESP32-C3', 'esp32'), false);
  assert.equal(DeviceInfo.isTargetCompatible('ESP8266', 'esp32'), false);
  assert.equal(DeviceInfo.isTargetCompatible(null, 'esp32'), false);
});

test('Flash Size Parsing', () => {
  assert.equal(DeviceInfo.parseFlashSizeBytes('4MB'), 4 * 1024 * 1024);
  assert.equal(DeviceInfo.parseFlashSizeBytes('8MB'), 8 * 1024 * 1024);
  assert.equal(DeviceInfo.parseFlashSizeBytes('16MB'), 16 * 1024 * 1024);
  assert.equal(DeviceInfo.parseFlashSizeBytes(null), 4 * 1024 * 1024);
});
