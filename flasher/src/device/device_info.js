/**
 * device_info.js
 * Hardware metadata formatting, register interpretation, and compatibility checks.
 * Explicitly designates ESP32-S3 as PRIMARY RELEASE TARGET.
 */

export class DeviceInfo {
  static getChipDisplayName(chipKey) {
    switch (chipKey?.toLowerCase()) {
      case 'esp32s3':
        return 'ESP32-S3-DevKitC-1 [PRIMARY RELEASE TARGET]';
      case 'esp32':
        return 'ESP32 Classic [FUTURE PORTING TARGET]';
      case 'esp32c3':
        return 'ESP32-C3 [FUTURE PORTING TARGET]';
      default:
        return 'ESP32 Generic';
    }
  }

  static isPrimaryTarget(chipKey) {
    return chipKey?.toLowerCase().replace(/[-_]/g, '') === 'esp32s3';
  }

  static isTargetCompatible(detectedKey, targetKey) {
    if (!detectedKey || !targetKey) return false;
    const d = detectedKey.toLowerCase().replace(/[-_]/g, '');
    const t = targetKey.toLowerCase().replace(/[-_]/g, '');
    return d === t;
  }

  static parseFlashSizeBytes(flashSizeStr) {
    if (!flashSizeStr) return 4 * 1024 * 1024;
    const upper = flashSizeStr.toUpperCase();
    if (upper.includes('8MB')) return 8 * 1024 * 1024;
    if (upper.includes('16MB')) return 16 * 1024 * 1024;
    if (upper.includes('4MB')) return 4 * 1024 * 1024;
    if (upper.includes('2MB')) return 2 * 1024 * 1024;
    return 4 * 1024 * 1024;
  }
}
