/**
 * dashboard.js
 * Main UI coordinator for the MicroKernel OS Web Serial Flasher.
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

    // UI Bindings
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
    this.console.log('[INFO] MicroKernel OS Flasher initialized.');

    if (!this.serial.isSupported()) {
      this.console.log('[ERROR] Web Serial API is NOT supported in this browser. Please use Chrome or Edge.', 'ERROR');
      this.notifications.setStatus('ERROR', 'Web Serial unavailable. Please switch to a Chromium browser.');
      this.btnConnect.disabled = true;
      return;
    }

    try {
      this.console.log('[INFO] Loading firmware release catalog...');
      await this.manifestMgr.load();
      const rel = this.manifestMgr.getLatestRelease();
      this.console.log(`[INFO] Loaded release v${rel.version} (${Object.keys(rel.targets).join(', ')})`);
      document.getElementById('appVersionBadge').textContent = `v${rel.version}`;
    } catch (e) {
      this.console.log(`[WARN] Manifest load note: ${e.message}`, 'WARN');
    }

    // Register disconnect listener
    this.serial.onDisconnect((err) => {
      this.handleDisconnect(err.message);
    });
  }

  bindEvents() {
    this.btnConnect.addEventListener('click', () => this.handleConnect());
    this.btnDisconnect.addEventListener('click', () => this.handleManualDisconnect());
    this.btnFlash.addEventListener('click', () => this.handleFlash());

    // Console filters
    document.querySelectorAll('.btn-filter').forEach(btn => {
      btn.addEventListener('click', (e) => {
        document.querySelectorAll('.btn-filter').forEach(b => b.classList.remove('active'));
        e.target.classList.add('active');
        this.console.setFilter(e.target.dataset.filter);
      });
    });

    document.getElementById('btnClearConsole').addEventListener('click', () => {
      this.console.clear();
    });
  }

  async handleConnect() {
    try {
      this.notifications.setStatus('CONNECTING', 'Requesting serial port authorization...');
      this.btnConnect.disabled = true;

      await this.serial.requestPort();
      const baud = parseInt(this.selBaud.value, 10) || 115200;

      const detector = new DeviceDetector(this.serial, (msg) => this.console.log(msg));
      const res = await detector.connectAndDetect(baud);

      this.detectedChip = res.chip;
      this.loader = res.loader;

      // Update UI
      this.valChipFamily.textContent = res.displayName;
      this.valChipRev.textContent = `Revision ${res.chip.revision}`;
      this.valFlashSize.textContent = res.chip.flashSize;
      this.valMacAddress.textContent = 'Auto-configured';

      // Match target configuration from manifest
      this.targetConfig = this.manifestMgr.getTargetConfig(res.chip.targetKey);
      if (!this.targetConfig) {
        throw new Error(`Target ${res.chip.family} is not supported in the current release manifest.`);
      }

      this.valTargetFirmware.textContent = `MicroKernel OS 1.0.0 (${this.targetConfig.chip})`;
      this.notifications.setStatus('READY', `Device ready: ${res.displayName}. Click 'Install MicroKernel OS'.`);
      this.btnDisconnect.disabled = false;
      this.btnFlash.disabled = false;

      this.console.log(`[INFO] Device verified successfully. Compatible firmware target: ${this.targetConfig.chip}`);
    } catch (err) {
      this.console.log(`[ERROR] Connection failed: ${err.message}`, 'ERROR');
      this.notifications.setStatus('ERROR', err.message);
      this.handleDisconnect();
    }
  }

  async handleFlash() {
    if (!this.targetConfig || !this.loader) {
      return;
    }

    try {
      this.btnFlash.disabled = true;
      this.btnDisconnect.disabled = true;
      this.notifications.setStatus('FLASHING', 'Flashing MicroKernel OS to ESP32 flash memory...');

      const firmwareMgr = new FirmwareManager('./firmware/', (msg) => this.console.log(msg));
      this.preparedImages = await firmwareMgr.fetchAndPrepareImages(this.targetConfig);

      this.progress.setupImages(this.preparedImages);

      const flashMgr = new FlashManager(
        this.loader,
        (msg) => this.console.log(msg),
        (telemetry) => this.progress.update(telemetry)
      );

      await flashMgr.flash(this.preparedImages, this.detectedChip);

      this.notifications.setStatus('READY', 'MicroKernel OS flashed successfully! Device running.');
      this.console.log('[INFO] Installation complete! The ESP32 executive is active and running.');
    } catch (err) {
      this.console.log(`[ERROR] Flashing aborted: ${err.message}`, 'ERROR');
      this.notifications.setStatus('ERROR', `Flashing failed: ${err.message}`);
    } finally {
      this.btnFlash.disabled = false;
      this.btnDisconnect.disabled = false;
    }
  }

  async handleManualDisconnect() {
    this.console.log('[INFO] Closing serial port connection...');
    await this.serial.close();
    this.handleDisconnect('Serial port disconnected by user.');
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
    this.loader = null;
    this.targetConfig = null;
  }
}
