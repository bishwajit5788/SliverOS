/**
 * main.js
 * Master orchestrator for SliverOS Web Host desktop interface.
 * Coordinates Web Serial communication, simulator, applications, canvas renderer,
 * interactive developer terminal, device monitor, and firmware flasher.
 */

import { SliverClient } from './protocol/sliver_client.js';
import { SliverSimulator } from './protocol/simulator.js';
import { RetroCanvasRenderer } from './ui/retro_canvas.js';
import { FlashManager } from './flashing/flash_manager.js';
import { FirmwareManager } from './flashing/firmware_manager.js';
import { ManifestManager } from './flashing/manifest.js';
import { DeviceDetector } from './device/device_detector.js';
import { WebSerialPort } from './serial/webserial.js';
import { DeviceInfo } from './device/device_info.js';

class SliverOSWebHost {
  constructor() {
    this.client = new SliverClient(this.log.bind(this));
    this.simulator = null;
    this.canvasRenderer = null;
    this.flasher = null;

    this.isSimulated = false;
    this.isThrusting = false;
    this.inputMask = 0; // Bit 0: UP, Bit 1: DOWN, Bit 2: LEFT, Bit 3: RIGHT, Bit 4: A, Bit 5: B
    this.cmdHistory = [];
    this.cmdHistoryIdx = -1;

    this.initDOMElements();
    this.initNavigation();
    this.initClientListeners();
    this.initGameEngine();
    this.initTerminal();
    this.initFlasher();
    this.initWifiView();
  }

  initDOMElements() {
    // Header controls
    this.btnConnect = document.getElementById('btnConnect');
    this.btnDisconnect = document.getElementById('btnDisconnect');
    this.btnSimulate = document.getElementById('btnSimulate');
    this.connectionPill = document.getElementById('connectionPill');
    this.connectionStatusLabel = document.getElementById('connectionStatusLabel');
    this.headerChipBadge = document.getElementById('headerChipBadge');
    this.desktopDeviceTag = document.getElementById('desktopDeviceTag');

    // Desktop metrics
    this.metricActiveApp = document.getElementById('metricActiveApp');
    this.metricActiveAppState = document.getElementById('metricActiveAppState');
    this.metricUptime = document.getElementById('metricUptime');
    this.metricTick = document.getElementById('metricTick');
    this.metricSramUsed = document.getElementById('metricSramUsed');
    this.barSram = document.getElementById('barSram');
    this.metricPsramFree = document.getElementById('metricPsramFree');

    // App state pills
    this.badgeAppBle = document.getElementById('badgeAppBle');
    this.badgeAppWifi = document.getElementById('badgeAppWifi');
    this.badgeAppNet = document.getElementById('badgeAppNet');
    this.badgeAppGames = document.getElementById('badgeAppGames');

    // Terminal
    this.terminalOutput = document.getElementById('terminalOutput');
    this.termForm = document.getElementById('termForm');
    this.termInput = document.getElementById('termInput');

    // Monitor
    this.tcbTableBody = document.getElementById('tcbTableBody');
    this.barMonitorSram = document.getElementById('barMonitorSram');
    this.barMonitorPsram = document.getElementById('barMonitorPsram');
    this.txtArenaUsed = document.getElementById('txtArenaUsed');
    this.txtArenaFree = document.getElementById('txtArenaFree');
    this.txtPsramUsed = document.getElementById('txtPsramUsed');
    this.txtPsramFree = document.getElementById('txtPsramFree');

    // Logs
    this.systemLogStream = document.getElementById('systemLogStream');
    this.btnClearLogs = document.getElementById('btnClearLogs');

    // Wi-Fi
    this.channelRow = document.getElementById('channelRow');
    this.wifiTotFrames = document.getElementById('wifiTotFrames');
    this.wifiMgmtFrames = document.getElementById('wifiMgmtFrames');
    this.wifiCtrlFrames = document.getElementById('wifiCtrlFrames');
    this.wifiDataFrames = document.getElementById('wifiDataFrames');
    this.wifiAvgRssi = document.getElementById('wifiAvgRssi');

    // Network Diag
    this.inputNetTarget = document.getElementById('inputNetTarget');
    this.btnRunNetDiag = document.getElementById('btnRunNetDiag');
    this.statusPort22 = document.getElementById('statusPort22');
    this.statusPort80 = document.getElementById('statusPort80');
    this.statusPort443 = document.getElementById('statusPort443');
    this.valIcmpStatus = document.getElementById('valIcmpStatus');

    // BLE Macro
    this.btnRunMacro = document.getElementById('btnRunMacro');
    this.btnStopMacro = document.getElementById('btnStopMacro');
    this.macroStatusText = document.getElementById('macroStatusText');

    // Flasher
    this.btnFlash = document.getElementById('btnFlash');
    this.safetyStatusText = document.getElementById('safetyStatusText');
    this.progressSection = document.getElementById('progressSection');
    this.barOverall = document.getElementById('barOverall');
    this.metricSpeed = document.getElementById('metricSpeed');
    this.metricTransferred = document.getElementById('metricTransferred');

    // Toast Shelf
    this.toastShelf = document.getElementById('toastShelf');

    // Bind Button Click Events
    this.btnConnect.addEventListener('click', () => this.handleConnect());
    this.btnDisconnect.addEventListener('click', () => this.handleDisconnect());
    this.btnSimulate.addEventListener('click', () => this.toggleSimulation());

    // Launch buttons on desktop
    document.querySelectorAll('.launch-btn').forEach(btn => {
      btn.addEventListener('click', e => {
        const appId = parseInt(e.target.getAttribute('data-app-id'), 10);
        this.switchAppTab(appId);
      });
    });

    if (this.btnClearLogs) {
      this.btnClearLogs.addEventListener('click', () => {
        this.systemLogStream.innerHTML = '';
      });
    }
  }

  initNavigation() {
    // Primary Sidebar Tabs
    const navItems = document.querySelectorAll('.nav-item');
    const stageViews = document.querySelectorAll('.stage-view');

    navItems.forEach(item => {
      item.addEventListener('click', () => {
        const tab = item.getAttribute('data-tab');
        navItems.forEach(n => n.classList.remove('active'));
        item.classList.add('active');

        stageViews.forEach(v => v.classList.remove('active'));
        const targetView = document.getElementById(`view${tab.charAt(0).toUpperCase() + tab.slice(1)}`);
        if (targetView) targetView.classList.add('active');
      });
    });

    // Sub-Nav Tabs in Applications view
    const subNavTabs = document.querySelectorAll('.sub-nav-tab');
    const subStages = document.querySelectorAll('.app-sub-stage');

    subNavTabs.forEach(tab => {
      tab.addEventListener('click', () => {
        const subtabId = tab.getAttribute('data-subtab');
        subNavTabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');

        subStages.forEach(s => s.classList.remove('active'));
        const targetStage = document.getElementById(subtabId);
        if (targetStage) targetStage.classList.add('active');
      });
    });
  }

  switchAppTab(appId) {
    // Switch main navigation to Apps view
    document.getElementById('navApps').click();

    // Switch sub-tab corresponding to appId
    const subNavTabs = document.querySelectorAll('.sub-nav-tab');
    const appSubIds = ['subBle', 'subWifi', 'subNet', 'subGames'];
    const targetSubId = appSubIds[appId] || 'subGames';

    subNavTabs.forEach(tab => {
      if (tab.getAttribute('data-subtab') === targetSubId) {
        tab.click();
      }
    });

    // Notify device or simulator
    if (this.client.isConnected) {
      if (this.isSimulated && this.simulator) {
        this.simulator.launchApp(appId);
      } else {
        this.client.launchApp(appId);
      }
    }
  }

  initClientListeners() {
    this.client.on('connectionStateChange', state => {
      this.updateConnectionUI(state);
    });

    this.client.on('deviceInfo', info => {
      this.updateDeviceInfoUI(info);
    });

    this.client.on('deviceStatus', status => {
      this.updateDeviceStatusUI(status);
    });

    this.client.on('memoryStatus', mem => {
      this.updateMemoryStatusUI(mem);
    });

    this.client.on('storageStatus', storage => {
      this.updateStorageStatusUI(storage);
    });

    this.client.on('gameState', game => {
      if (this.canvasRenderer) {
        this.canvasRenderer.updateState(game);
        this.updateGameHudUI(game);
      }
    });

    this.client.on('appState', app => {
      this.updateAppStateUI(app);
    });

    this.client.on('log', log => {
      this.appendLog(log);
    });

    this.client.on('terminalOutput', text => {
      this.appendTerminal(text);
    });
  }

  async handleConnect() {
    try {
      if (this.isSimulated) {
        this.toggleSimulation(); // Exit simulation first
      }
      this.showToast('Connecting via Web Serial... Select ESP32-S3');
      await this.client.requestDeviceAndConnect(115200);
      this.showToast('Connected to SliverOS ESP32-S3!');
    } catch (err) {
      if (err.name !== 'NotFoundError') {
        this.showToast(`Connection failed: ${err.message}`, 'error');
      }
    }
  }

  async handleDisconnect() {
    if (this.isSimulated && this.simulator) {
      this.simulator.stop();
      this.isSimulated = false;
      this.btnSimulate.classList.remove('active');
      this.btnSimulate.innerHTML = '<span class="btn-icon">🧪</span> SIMULATE DEVICE';
    } else {
      await this.client.disconnect();
    }
    this.showToast('Device disconnected.');
  }

  toggleSimulation() {
    if (this.isSimulated) {
      if (this.simulator) {
        this.simulator.stop();
        this.simulator = null;
      }
      this.isSimulated = false;
      this.btnSimulate.classList.remove('active');
      this.btnSimulate.innerHTML = '<span class="btn-icon">🧪</span> SIMULATE DEVICE';
      this.showToast('Simulator stopped.');
    } else {
      if (this.client.isConnected) {
        this.client.disconnect();
      }
      this.isSimulated = true;
      this.btnSimulate.classList.add('active');
      this.btnSimulate.innerHTML = '<span class="btn-icon">🛑</span> STOP SIMULATOR';
      this.simulator = new SliverSimulator(this.client);
      this.simulator.start();
      this.showToast('SliverOS Simulated Device running!');
    }
  }

  updateConnectionUI(state) {
    this.connectionPill.className = 'connection-status-pill ' + state;

    if (state === 'connected') {
      this.connectionStatusLabel.textContent = 'CONNECTED (USB)';
      this.btnConnect.style.display = 'none';
      this.btnDisconnect.style.display = 'inline-flex';
      this.btnSimulate.style.display = 'none';
    } else if (state === 'simulated') {
      this.connectionStatusLabel.textContent = 'SIMULATED DEVICE';
      this.btnConnect.style.display = 'none';
      this.btnDisconnect.style.display = 'none';
      this.btnSimulate.style.display = 'inline-flex';
    } else {
      this.connectionStatusLabel.textContent = 'DISCONNECTED';
      this.btnConnect.style.display = 'inline-flex';
      this.btnDisconnect.style.display = 'none';
      this.btnSimulate.style.display = 'inline-flex';
      this.desktopDeviceTag.textContent = 'Waiting for connection...';
      this.headerChipBadge.textContent = '7Semi ESP32-S3-N8R8';
    }
  }

  updateDeviceInfoUI(info) {
    if (!info) return;
    const isSim = info.isSimulated;
    const tag = isSim
      ? 'SIMULATED DEVICE (No physical hardware)'
      : `${info.chipFamily} Rev ${info.chipRevision} • ${info.flashSizeBytes / (1024*1024)}MB Flash • ${info.psramSizeBytes / (1024*1024)}MB PSRAM`;

    this.desktopDeviceTag.textContent = tag;
    this.headerChipBadge.textContent = isSim ? 'ESP32-S3 (SIMULATED)' : `${info.chipFamily} Rev ${info.chipRevision}`;
  }

  updateDeviceStatusUI(status) {
    if (!status) return;

    this.metricUptime.textContent = `${status.uptimeSeconds} s`;
    this.metricTick.textContent = `Tick: ${status.tick}`;

    const appNames = ['BLE-HID Macro', 'Wi-Fi Diagnostics', 'Network Diagnostics', 'Retro Games'];
    const curName = appNames[status.activeApp] || `App ${status.activeApp}`;
    this.metricActiveApp.textContent = curName;
    this.metricActiveAppState.textContent = `Kernel: ${status.kernelState === 3 ? 'RUNNING' : 'READY'} • Iter: ${status.schedulerIterations}`;

    // Update app cards state pills
    this.badgeAppBle.textContent = (status.activeApp === 0) ? 'ACTIVE' : 'READY';
    this.badgeAppBle.className = 'app-state-pill ' + ((status.activeApp === 0) ? 'active' : '');

    this.badgeAppWifi.textContent = (status.activeApp === 1) ? 'ACTIVE' : 'READY';
    this.badgeAppWifi.className = 'app-state-pill ' + ((status.activeApp === 1) ? 'active' : '');

    this.badgeAppNet.textContent = (status.activeApp === 2) ? 'ACTIVE' : 'READY';
    this.badgeAppNet.className = 'app-state-pill ' + ((status.activeApp === 2) ? 'active' : '');

    this.badgeAppGames.textContent = (status.activeApp === 3) ? 'ACTIVE' : 'READY';
    this.badgeAppGames.className = 'app-state-pill ' + ((status.activeApp === 3) ? 'active' : '');

    // Update Monitor TCB Table
    if (status.tasks && status.tasks.length > 0) {
      const stateMap = ['UNUSED', 'READY', 'RUN', 'BLOCK', 'SLEEP', 'TERM', 'FAULT'];
      let html = '';
      for (const t of status.tasks) {
        html += `
          <tr>
            <td>${t.id}</td>
            <td><strong>${t.name}</strong></td>
            <td><span class="badge ${t.state === 2 ? 'badge-safe' : ''}">${stateMap[t.state] || 'UNK'}</span></td>
            <td>${t.priority}</td>
            <td>${t.periodTicks}</td>
            <td>${t.executionCount}</td>
            <td>${t.lastExecutionUs} µs</td>
            <td>${t.worstExecutionUs} µs</td>
            <td><span style="color: ${t.overrunCount > 0 ? '#ef4444' : '#94a3b8'}">${t.overrunCount}</span></td>
          </tr>
        `;
      }
      this.tcbTableBody.innerHTML = html;
    }
  }

  updateMemoryStatusUI(mem) {
    if (!mem) return;

    const usedKb = Math.round(mem.arenaUsed / 1024);
    const capKb = Math.round(mem.arenaCapacity / 1024);
    const pct = Math.min(100, Math.round((mem.arenaUsed / (mem.arenaCapacity || 1)) * 100));

    this.metricSramUsed.textContent = `${usedKb} / ${capKb} KB (${pct}%)`;
    this.barSram.style.width = `${pct}%`;
    this.barMonitorSram.style.width = `${pct}%`;
    this.txtArenaUsed.textContent = `Used: ${mem.arenaUsed} B (Peak: ${mem.arenaPeak} B)`;
    this.txtArenaFree.textContent = `Free: ${mem.arenaFree} B`;

    const psramFreeMb = (mem.psramFree / (1024 * 1024)).toFixed(1);
    this.metricPsramFree.textContent = `${psramFreeMb} MB Free`;
    const psramPct = Math.min(100, Math.round(((mem.psramTotal - mem.psramFree) / (mem.psramTotal || 1)) * 100));
    this.barMonitorPsram.style.width = `${psramPct}%`;
    this.txtPsramUsed.textContent = `Used: ${mem.psramTotal - mem.psramFree} B`;
    this.txtPsramFree.textContent = `Free: ${mem.psramFree} B`;
  }

  updateStorageStatusUI(storage) {
    if (!storage) return;
    const vfsTot = document.getElementById('vfsTotSectors');
    if (vfsTot) {
      vfsTot.textContent = storage.totalSectors;
    }
  }

  updateGameHudUI(game) {
    document.getElementById('hudFuel').textContent = game.fuel;
    document.getElementById('hudScore').textContent = game.score;
    document.getElementById('hudVy').textContent = (game.vy / 10).toFixed(1);
    document.getElementById('hudVx').textContent = (game.vx / 10).toFixed(1);
    document.getElementById('valGameFrameSeq').textContent = game.frameSeq;

    const gameStatusTag = document.getElementById('gameStatusTag');
    const overlay = document.getElementById('gameOverlay');

    if (game.status === 0) {
      gameStatusTag.textContent = 'IN FLIGHT';
      gameStatusTag.style.color = '#00f5ff';
      overlay.style.display = 'none';
    } else if (game.status === 1) {
      gameStatusTag.textContent = 'LANDED';
      gameStatusTag.style.color = '#00f5a0';
      overlay.style.display = 'flex';
      document.getElementById('overlayTitle').textContent = 'MISSION SUCCESSFUL!';
      document.getElementById('overlayMessage').textContent = `Lander touched down safely. Score: ${game.score}`;
      document.getElementById('btnStartGame').textContent = 'NEXT FLIGHT';
    } else if (game.status === 2) {
      gameStatusTag.textContent = 'CRASHED';
      gameStatusTag.style.color = '#ef4444';
      overlay.style.display = 'flex';
      document.getElementById('overlayTitle').textContent = 'CRITICAL IMPACT!';
      document.getElementById('overlayMessage').textContent = 'Descent velocity exceeded landing gear limits.';
      document.getElementById('btnStartGame').textContent = 'TRY AGAIN';
    }
  }

  updateAppStateUI(app) {
    if (app.appId === 1 && app.wifiStats) {
      const s = app.wifiStats;
      this.wifiTotFrames.textContent = s.totalFrames;
      this.wifiMgmtFrames.textContent = s.mgmtFrames;
      this.wifiCtrlFrames.textContent = s.ctrlFrames;
      this.wifiDataFrames.textContent = s.dataFrames;
      this.wifiAvgRssi.textContent = `${s.avgRssi} dBm`;
      this.setActiveChannelBox(s.channel);
    }
  }

  initWifiView() {
    this.channelRow.innerHTML = '';
    for (let c = 1; c <= 11; c++) {
      const box = document.createElement('div');
      box.className = 'channel-box' + (c === 1 ? ' active' : '');
      box.id = `chanBox_${c}`;
      box.innerHTML = `<span style="font-size:9px;color:#64748b">CH</span><br><strong>${c}</strong>`;
      this.channelRow.appendChild(box);
    }
  }

  setActiveChannelBox(activeChan) {
    for (let c = 1; c <= 11; c++) {
      const el = document.getElementById(`chanBox_${c}`);
      if (el) {
        if (c === activeChan) {
          el.classList.add('active');
        } else {
          el.classList.remove('active');
        }
      }
    }
  }

  initGameEngine() {
    const canvas = document.getElementById('gameCanvas');
    if (!canvas) return;

    this.canvasRenderer = new RetroCanvasRenderer(canvas);

    // Setup input listeners
    const handleKey = (e, isDown) => {
      let mask = 0;
      let handled = false;

      if (e.key === 'ArrowUp' || e.key === 'w' || e.key === 'W') {
        mask = 0x01; // UP
        this.isThrusting = isDown;
        handled = true;
      } else if (e.key === 'ArrowLeft' || e.key === 'a' || e.key === 'A') {
        mask = 0x04; // LEFT
        handled = true;
      } else if (e.key === 'ArrowRight' || e.key === 'd' || e.key === 'D') {
        mask = 0x08; // RIGHT
        handled = true;
      } else if (e.key === ' ' || e.key === 'r' || e.key === 'R') {
        mask = 0x10; // BUTTON A (RESET)
        handled = true;
      }

      if (handled) {
        e.preventDefault();
        if (isDown) {
          this.inputMask |= mask;
        } else {
          this.inputMask &= ~mask;
        }
        this.dispatchAppInput(3, this.inputMask);
      }
    };

    window.addEventListener('keydown', e => handleKey(e, true));
    window.addEventListener('keyup', e => handleKey(e, false));

    // On-screen touch/click control buttons
    const bindBtn = (id, maskBit) => {
      const el = document.getElementById(id);
      if (!el) return;
      const setMask = down => {
        if (down) {
          this.inputMask |= maskBit;
          if (maskBit === 0x01) this.isThrusting = true;
        } else {
          this.inputMask &= ~maskBit;
          if (maskBit === 0x01) this.isThrusting = false;
        }
        this.dispatchAppInput(3, this.inputMask);
      };

      el.addEventListener('mousedown', () => setMask(true));
      el.addEventListener('mouseup', () => setMask(false));
      el.addEventListener('mouseleave', () => setMask(false));
      el.addEventListener('touchstart', e => { e.preventDefault(); setMask(true); });
      el.addEventListener('touchend', e => { e.preventDefault(); setMask(false); });
    };

    bindBtn('btnCtrlThrust', 0x01);
    bindBtn('btnCtrlLeft', 0x04);
    bindBtn('btnCtrlRight', 0x08);
    bindBtn('btnCtrlReset', 0x10);

    const btnStartGame = document.getElementById('btnStartGame');
    if (btnStartGame) {
      btnStartGame.addEventListener('click', () => {
        document.getElementById('gameOverlay').style.display = 'none';
        this.dispatchAppInput(3, 0x10); // Reset
      });
    }

    // Animation frame render loop
    const animate = () => {
      if (this.canvasRenderer) {
        this.canvasRenderer.render(this.isThrusting);
      }
      requestAnimationFrame(animate);
    };
    requestAnimationFrame(animate);
  }

  dispatchAppInput(appId, mask) {
    if (this.isSimulated && this.simulator) {
      this.simulator.setInput(mask);
    } else if (this.client.isConnected) {
      this.client.sendAppInput(appId, mask).catch(() => {});
    }
  }

  initTerminal() {
    this.termForm.addEventListener('submit', e => {
      e.preventDefault();
      const val = this.termInput.value;
      if (!val.trim()) return;

      this.cmdHistory.push(val);
      this.cmdHistoryIdx = this.cmdHistory.length;

      this.appendTerminal(`root@sliver:~# ${val}\r\n`);
      this.termInput.value = '';

      if (this.isSimulated && this.simulator) {
        this.simulator.handleTerminal(val);
      } else if (this.client.isConnected) {
        this.client.sendTerminalCommand(val).catch(err => {
          this.appendTerminal(`[ERROR] Send failed: ${err.message}\r\n`);
        });
      } else {
        this.appendTerminal('[WARN] Device not connected. Connect via USB or start Simulator to execute shell commands.\r\n');
      }
    });

    // Terminal History navigation
    this.termInput.addEventListener('keydown', e => {
      if (e.key === 'ArrowUp') {
        if (this.cmdHistory.length > 0 && this.cmdHistoryIdx > 0) {
          this.cmdHistoryIdx--;
          this.termInput.value = this.cmdHistory[this.cmdHistoryIdx];
        }
      } else if (e.key === 'ArrowDown') {
        if (this.cmdHistoryIdx < this.cmdHistory.length - 1) {
          this.cmdHistoryIdx++;
          this.termInput.value = this.cmdHistory[this.cmdHistoryIdx];
        } else {
          this.cmdHistoryIdx = this.cmdHistory.length;
          this.termInput.value = '';
        }
      }
    });

    // Terminal Shortcut Chips
    document.querySelectorAll('.term-btn-chip').forEach(btn => {
      btn.addEventListener('click', () => {
        const cmd = btn.getAttribute('data-cmd');
        this.termInput.value = cmd;
        this.termForm.dispatchEvent(new Event('submit'));
      });
    });
  }

  appendTerminal(text) {
    if (text.includes('\x1b[2J')) {
      this.terminalOutput.innerHTML = '';
      return;
    }
    const span = document.createElement('span');
    span.textContent = text;
    this.terminalOutput.appendChild(span);
    this.terminalOutput.scrollTop = this.terminalOutput.scrollHeight;
  }

  appendLog(log) {
    const line = document.createElement('div');
    line.className = 'log-line';
    const timeSec = (log.timestampMs / 1000).toFixed(3);
    line.innerHTML = `
      <span class="log-time">[${timeSec}]</span>
      <span class="log-tag">[SLIVER]</span>
      <span class="log-msg">${this.escapeHTML(log.text)}</span>
    `;
    this.systemLogStream.appendChild(line);
    this.systemLogStream.scrollTop = this.systemLogStream.scrollHeight;
  }

  escapeHTML(str) {
    return (str || '').replace(/[&<>'"]/g, tag => ({
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      "'": '&#39;',
      '"': '&quot;'
    }[tag] || tag));
  }

  showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    this.toastShelf.appendChild(toast);
    setTimeout(() => {
      toast.remove();
    }, 3500);
  }

  log(msg) {
    console.log(msg);
    this.appendLog({ timestampMs: Date.now(), text: msg });
  }

  initFlasher() {
    this.btnFlash.addEventListener('click', async () => {
      if (!navigator.serial) {
        this.showToast('Web Serial is not supported in this browser. Please use Chrome, Edge, or Opera.', 'error');
        return;
      }

      this.progressSection.style.display = 'block';
      this.safetyStatusText.textContent = 'Requesting serial port authorization for bootloader...';
      let serialPort = null;

      try {
        if (this.client.isConnected) {
          await this.client.disconnect();
        }

        serialPort = new WebSerialPort();
        await serialPort.requestPort();

        this.safetyStatusText.textContent = 'Synchronizing with ROM bootloader...';
        const detector = new DeviceDetector(serialPort, (msg) => this.log(msg));
        const res = await detector.connectAndDetect(115200);

        this.log(`[INFO] Connected to ROM Bootloader: ${res.displayName}`);

        // STRICT TARGET ENFORCEMENT: ESP32-S3 ONLY
        if (!DeviceInfo.isSupportedTarget(res.chip.targetKey)) {
          throw new Error(
            `Target chip '${res.chip.family}' (${res.displayName}) is NOT supported! ` +
            `SliverOS flasher strictly requires ESP32-S3 (7Semi ESP32-S3-Dev-BoardC-1U-N8R8). ` +
            `Flashing rejected to protect non-target hardware.`
          );
        }

        this.safetyStatusText.textContent = `Target verified: ${res.displayName}. Fetching release manifest...`;
        const manifestMgr = new ManifestManager();
        await manifestMgr.load();

        const targetConfig = manifestMgr.getTargetConfig(res.chip.targetKey);
        if (!targetConfig) {
          throw new Error(`Target ${res.chip.family} not found in firmware manifest.`);
        }

        this.safetyStatusText.textContent = 'Fetching and verifying firmware images (SHA-256 integrity check)...';
        const firmwareMgr = new FirmwareManager('./firmware/', (msg) => this.log(msg), false);
        const images = await firmwareMgr.fetchAndPrepareImages(targetConfig);

        this.safetyStatusText.textContent = 'Writing firmware images to ESP32-S3 flash...';
        const flashMgr = new FlashManager(
          res.loader,
          (msg) => this.log(msg),
          (telemetry) => {
            this.barOverall.style.width = `${telemetry.overallPct}%`;
            this.metricSpeed.textContent = `${telemetry.speedKBps} KB/s`;
            this.metricTransferred.textContent = `${Math.round(telemetry.bytesTransferred / 1024)} KB`;
            this.safetyStatusText.textContent = `Writing ${telemetry.imageName} (${telemetry.overallPct}% complete)...`;
          }
        );

        await flashMgr.flash(images, res.chip);
        this.safetyStatusText.textContent = 'Firmware installation complete! Restarting SliverOS...';
        this.showToast('Firmware installed successfully!', 'success');
      } catch (err) {
        this.safetyStatusText.textContent = `Flashing halted: ${err.message}`;
        this.showToast(`Flasher error: ${err.message}`, 'error');
      } finally {
        if (serialPort && serialPort.isOpen) {
          await serialPort.close();
        }
      }
    });
  }
}

// Instantiate on DOM load
window.addEventListener('DOMContentLoaded', () => {
  window.sliverOS = new SliverOSWebHost();
});
