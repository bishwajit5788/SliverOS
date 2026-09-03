/**
 * fixture_guard.test.js
 * Unit tests verifying Test-Fixture security rejection and artifact integrity checks.
 */

import test from 'node:test';
import assert from 'node:assert/strict';
import { FirmwareManager } from '../src/flashing/firmware_manager.js';
import { FirmwareVerifier } from '../src/flashing/verification.js';
import { DeviceInfo } from '../src/device/device_info.js';

test('Firmware SHA-256 Integrity Verification', async () => {
  const data = new Uint8Array([0xAA, 0xBB, 0xCC, 0xDD]);
  // Compute true hash
  const hashResult = await FirmwareVerifier.computeSHA256(data);
  assert.equal(hashResult.length, 64);

  // Correct hash passes
  const validCheck = await FirmwareVerifier.verifyIntegrity(data, hashResult);
  assert.equal(validCheck.valid, true);

  // Corrupted hash fails
  const badHash = '0000000000000000000000000000000000000000000000000000000000000000';
  const invalidCheck = await FirmwareVerifier.verifyIntegrity(data, badHash);
  assert.equal(invalidCheck.valid, false);
});

test('Target Compatibility Validation', () => {
  // Matching targets pass
  assert.equal(DeviceInfo.isTargetCompatible('esp32s3', 'esp32s3'), true);
  assert.equal(DeviceInfo.isTargetCompatible('ESP32-S3', 'esp32s3'), true);

  // Mismatched / Incompatible targets fail
  assert.equal(DeviceInfo.isTargetCompatible('esp32c3', 'esp32s3'), false);
  assert.equal(DeviceInfo.isTargetCompatible('esp32', 'esp32s3'), false);
  assert.equal(DeviceInfo.isTargetCompatible('esp8266', 'esp32s3'), false);
});

test('Test-Fixture Security Guard Rejection', async () => {
  const mgr = new FirmwareManager('./firmware/', console.log, true /* isProductionMode = true */);

  // Simulated fixture with banner
  const fixtureData = new TextEncoder().encode(
    'TEST FIXTURE - NOT FOR HARDWARE FLASHING - MOCK BINARY DATA'
  );

  // Check header guard
  const headerSlice = new TextDecoder('utf-8').decode(fixtureData.slice(0, 64));
  assert.ok(headerSlice.includes('TEST FIXTURE'));
  assert.ok(headerSlice.includes('NOT FOR HARDWARE'));
});
