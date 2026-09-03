/**
 * flash_manager.js
 * Master flashing engine orchestrating safety validation, block writing,
 * real-time transfer telemetry, and post-flash hardware verification.
 */

export class FlashManager {
  constructor(loader, logger = console.log, progressCallback = () => {}) {
    this.loader = loader;
    this.log = logger;
    this.onProgress = progressCallback;
    this.isFlashing = false;
  }

  async flash(images, targetChip) {
    if (this.isFlashing) {
      throw new Error('Flash operation already in progress.');
    }
    if (!images || images.length === 0) {
      throw new Error('No images specified for flashing.');
    }

    this.isFlashing = true;
    const totalBytes = images.reduce((sum, img) => sum + img.size, 0);
    let bytesTransferred = 0;
    const startTime = Date.now();

    this.log(`[INFO] Starting flash sequence: ${images.length} images, total ${totalBytes} bytes`);

    try {
      for (let imgIdx = 0; imgIdx < images.length; imgIdx++) {
        const img = images[imgIdx];
        this.log(`[INFO] Writing image [${imgIdx + 1}/${images.length}]: ${img.name} to 0x${img.offset.toString(16)} (${img.size} bytes)...`);

        const blockSize = 4096;
        const totalBlocks = Math.ceil(img.size / blockSize);

        // Flash begin (erases required sectors in hardware)
        await this.loader.flashBegin(img.size, img.offset, blockSize);

        let imgBytesWritten = 0;

        for (let blockIdx = 0; blockIdx < totalBlocks; blockIdx++) {
          const start = blockIdx * blockSize;
          const end = Math.min(start + blockSize, img.size);
          const chunk = img.data.subarray(start, end);

          // Write chunk via bootloader protocol
          await this.loader.flashData(chunk, blockIdx);

          imgBytesWritten += chunk.length;
          bytesTransferred += chunk.length;

          const now = Date.now();
          const elapsedSec = (now - startTime) / 1000;
          const speedKBps = elapsedSec > 0 ? (bytesTransferred / 1024 / elapsedSec).toFixed(1) : 0;
          const remainingBytes = totalBytes - bytesTransferred;
          const remainingSec = speedKBps > 0 ? Math.ceil(remainingBytes / (speedKBps * 1024)) : 0;

          const overallPct = Math.round((bytesTransferred / totalBytes) * 100);
          const imagePct = Math.round((imgBytesWritten / img.size) * 100);

          this.onProgress({
            overallPct,
            imagePct,
            imageIndex: imgIdx,
            imageName: img.name,
            bytesTransferred,
            totalBytes,
            speedKBps,
            remainingSec
          });
        }

        // Finalize flash for current image
        await this.loader.flashEnd(false);

        // Post-write verification using ROM MD5 check
        this.log(`[INFO] Verifying image ${img.name} on flash...`);
        await this.loader.verifyFlashMD5(img.offset, img.size);
      }

      this.log('[INFO] All firmware images successfully written and verified.');

      // Hard reset to start OS
      await this.loader.hardReset();
      this.log('[INFO] Target device rebooted into MicroKernel OS.');

      return true;
    } finally {
      this.isFlashing = false;
    }
  }
}
