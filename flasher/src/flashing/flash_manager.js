/**
 * flash_manager.js
 * Flashing orchestration built on Espressif's official esptool-js ROM client.
 */

export class FlashManager {
  constructor(loader, logger = console.log, progressCallback = () => {}) {
    this.loader = loader;
    this.log = logger;
    this.onProgress = progressCallback;
    this.isFlashing = false;
  }

  async flash(images, targetChip, targetConfig = {}) {
    if (this.isFlashing) throw new Error('Flash operation already in progress.');
    if (!this.loader || !Array.isArray(images) || images.length === 0) {
      throw new Error('No valid firmware images are available for flashing.');
    }

    this.isFlashing = true;
    try {
      const totalBytes = images.reduce((sum, image) => sum + image.size, 0);
      let transferred = 0;

      this.log(`[INFO] Preparing ${images.length} verified firmware image(s) for ${targetChip.family}.`);

      const fileArray = images.map((image) => ({
        data: image.data,
        address: image.offset
      }));

      const flashOptions = {
        fileArray,
        flashMode: targetConfig.flash_mode || 'dio',
        flashFreq: targetConfig.flash_freq || '40m',
        flashSize: targetConfig.flash_size || 'detect',
        eraseAll: targetConfig.erase_all === true,
        compress: false,
        reportProgress: (fileIndex, written, total) => {
          const current = images[fileIndex];
          const previousFiles = images.slice(0, fileIndex).reduce((sum, image) => sum + image.size, 0);
          transferred = previousFiles + written;
          const overallPct = totalBytes > 0 ? Math.round((transferred / totalBytes) * 100) : 0;
          const imagePct = total > 0 ? Math.round((written / total) * 100) : 0;
          this.onProgress({
            overallPct,
            imagePct,
            imageIndex: fileIndex,
            imageName: current?.name || `image-${fileIndex + 1}`,
            bytesTransferred: transferred,
            totalBytes,
            speedKBps: 0,
            remainingSec: 0
          });
        }
      };

      this.log('[INFO] Writing firmware using Espressif ROM flashing protocol...');
      await this.loader.writeFlash(flashOptions);

      this.onProgress({
        overallPct: 100,
        imagePct: 100,
        imageIndex: images.length - 1,
        imageName: images[images.length - 1].name,
        bytesTransferred: totalBytes,
        totalBytes,
        speedKBps: 0,
        remainingSec: 0
      });

      this.log('[INFO] ROM reported successful flash completion.');
      await this.loader.after('hard_reset');
      this.log('[INFO] ESP32-S3 reset. SliverOS should now boot from flash.');
      return true;
    } finally {
      this.isFlashing = false;
    }
  }
}
