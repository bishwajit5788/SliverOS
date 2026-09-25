/**
 * dashboard.js
 * Main UI coordinator for the beginner-first SliverOS Web Serial Flasher.
 */

import { WebSerialPort } from '../serial/webserial.js';
import { DeviceDetector } from '../device/device_detector.js';
import { ManifestManager } from '../flashing/manifest.js';
import { FirmwareManager } from '../flashing/firmware_manager.js';
import { FlashManager } from '../flashing/flash_manager.js';
import { ConsoleUI } from './console.js';
import { ProgressUI } from './progress.js';
import { NotificationUI } from './notifications.js';

export class Dashboard {
  constructor() {
    this.serial = new WebSerialPort();
    this.manifestMgr = new ManifestManager();
    this.detectedChip = null;
    this.targetConfig = null;
    this.preparedImages = null;
    this.loader = null;
    this.transport = null;

    this.console = new ConsoleUI(document.getElementById('consoleOutput'));
    this.progress = new ProgressUI(
      document.getElementById('barOverall'),
      document.getElementById('lblOverallPct'),
      document.getElementById('lblOverallStep'),
      document.getElementById('progressMetrics'),
      document.getElementById('imageProgressContainer')
    );
    this.notifications = new NotificationUI(
      document.getElementById('headerStatusBadge'),
      document.getElementById('safetyStatusText')
    );

    this.btnConnect = document.getElementById('btnConnect');
    this.btnDisconnect = document.getElementById('btnDisconnect');
    this.btnFlash = document.getElementById('btnFlash');
    this.selBaud = document.getElementById('selBaudrate');
    this.valChipFamily = document.getElementById('valChipFamily');
    this.valChipRev = document.getElementById('valChipRev');
    this.valFlashSize = document.getElementById('valFlashSize');
    this.valMacAddress = document.getElementById('valMacAddress');
    this.valTargetFirmware = document.getElementById('valTargetFirmware');
  }

  async init() {
    this.bindEvents();
    this.console.log('[INFO] SliverOS Flasher initialized.');

    if (!this.serial.isSupported()) {
      this.notifications.setStatus('ERROR', 'Web Serial is unavailable. Use Chrome or Edge on a desktop.');
      this.btnConnect.disabled = true;
      return;
    }

    try {
      await this.manifestMgr.load();
      const rel = this.manifestMgr.getLatestRelease();
      this.console.log(`[INFO] Firmware catalog loaded: v${rel.version}`);
      document.getElementById('appVersionBadge').textContent = `v${rel.version}`;
    } catch (error) {
      this.console.log(`[WARN] Firmware catalog unavailable: ${error.message}`, 'WARN');
    }

    this.serial.onDisconnect((err) => this.handleDisconnect(err.message));
  }

  bindEvents() {
    this.btnConnect.addEventListener('click', () => this.handleConnect());
    this.btnDisconnect.addEventListener('click', () => this.handleManualDisconnect());
    this.btnFlash.addEventListener('click', () => this.handleFlash());

    document.querySelectorAll('.btn-filter').forEach(btn => {
      btn.addEventListener('click', (e) => {
        document.querySelectorAll('.btn-filter').forEach(b => b.classList.remove('active'));
        e.target.classList.add('active');
        this.console.setFilter(e.target.dataset.filter);
      });
    });
    document.getElementById('btnClearConsole').addEventListener('click', () => this.console.clear());
  }

  async handleConnect() {
    try {
      this.notifications.setStatus('CONNECTING', 'Choose your ESP32-S3 USB device...');
      this.btnConnect.disabled = true;
      await this.serial.requestPort();

      const baud = parseInt(this.selBaud.value, 10) || 115200;
      const detector = new DeviceDetector(this.serial, (msg) => this.console.log(msg));
      const res = await detector.connectAndDetect(baud);

      this.detectedChip = res.chip;
      this.loader = res.loader;
      this.transport = res.transport;

      this.valChipFamily.textContent = res.displayName;
      this.valChipRev.textContent = `Revision ${res.chip.revision}`;
      this.valFlashSize.textContent = res.chip.flashSize;
      this.valMacAddress.textContent = 'Detected by ROM';

      this.targetConfig = this.manifestMgr.getTargetConfig(res.chip.targetKey);
      if (!this.targetConfig) {
        throw new Error('No compatible SliverOS ESP32-S3 firmware release is published yet.');
      }

      this.valTargetFirmware.textContent = `SliverOS ${this.targetConfig.version || 'latest'} (${this.targetConfig.chip})`;
      this.notifications.setStatus('READY', 'ESP32-S3 ready. Click Install SliverOS.');
      this.btnDisconnect.disabled = false;
      this.btnFlash.disabled = false;
      this.console.log('[INFO] ESP32-S3 ROM connection and compatibility check passed.');
    } catch (error) {
      this.console.log(`[ERROR] Connection failed: ${error.message}`, 'ERROR');
      this.notifications.setStatus('ERROR', error.message);
      await this.closeTransport();
      this.handleDisconnect();
    }
  }

  async handleFlash() {
    if (!this.targetConfig || !this.loader) return;

    try {
      this.btnFlash.disabled = true;
      this.btnDisconnect.disabled = true;
      this.notifications.setStatus('FLASHING', 'Installing SliverOS... Do not unplug the board.');

      const firmwareMgr = new FirmwareManager('./firmware/', (msg) => this.console.log(msg));
      this.preparedImages = await firmwareMgr.fetchAndPrepareImages(this.targetConfig);
      this.progress.setupImages(this.preparedImages);

      const flashMgr = new FlashManager(
        this.loader,
        (msg) => this.console.log(msg),
        (telemetry) => this.progress.update(telemetry)
      );

      await flashMgr.flash(this.preparedImages, this.detectedChip, this.targetConfig);
      this.notifications.setStatus('READY', 'SliverOS installed. The ESP32-S3 is rebooting.');
      this.console.log('[INFO] Installation complete. USB may now be disconnected.');
    } catch (error) {
      this.console.log(`[ERROR] Installation failed: ${error.message}`, 'ERROR');
      this.notifications.setStatus('ERROR', `Installation failed: ${error.message}`);
    } finally {
      this.btnFlash.disabled = false;
      this.btnDisconnect.disabled = false;
    }
  }

  async closeTransport() {
    try {
      if (this.transport && typeof this.transport.disconnect === 'function') {
        await this.transport.disconnect();
      }
    } catch (error) {
      this.console.log(`[WARN] USB disconnect cleanup: ${error.message}`, 'WARN');
    }
    this.transport = null;
    this.loader = null;
  }

  async handleManualDisconnect() {
    this.console.log('[INFO] Disconnecting ESP32-S3...');
    await this.closeTransport();
    this.serial.port = null;
    this.handleDisconnect('Device disconnected by user.');
  }

  handleDisconnect(msg = 'Device disconnected.') {
    this.notifications.setStatus('DISCONNECTED', msg);
    this.btnConnect.disabled = false;
    this.btnDisconnect.disabled = true;
    this.btnFlash.disabled = true;
    this.valChipFamily.textContent = '—';
    this.valChipRev.textContent = '—';
    this.valFlashSize.textContent = '—';
    this.valMacAddress.textContent = '—';
    this.valTargetFirmware.textContent = '—';
    this.detectedChip = null;
    this.targetConfig = null;
    this.preparedImages = null;
  }
}
