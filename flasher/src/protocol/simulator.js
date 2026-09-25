/**
 * simulator.js
 * In-browser SliverOS Host Protocol Simulator.
 * Emulates the 7Semi ESP32-S3 firmware, cooperative scheduler, 4 applications,
 * Space Micro-Lander physics, memory metrics, and interactive terminal.
 *
 * Clearly marks all emitted telemetry as SIMULATED.
 */

import { MsgType } from './host_protocol.js';

export class SliverSimulator {
  constructor(client) {
    this.client = client;
    this.timer = null;
    this.gameTimer = null;
    this.tick = 1000;
    this.activeApp = 3; // Start on Retro Games by default or 0
    this.appStates = [
      { id: 0, name: 'BLE_HID', state: 2, runs: 12, errors: 0 },
      { id: 1, name: 'WIFI_DIAG', state: 2, runs: 45, errors: 0 },
      { id: 2, name: 'NET_DIAG', state: 2, runs: 6, errors: 0 },
      { id: 3, name: 'RETRO_GAMES', state: 3, runs: 28, errors: 0 }, // ACTIVE
    ];

    // Space Micro-Lander Physics State
    this.lander = {
      x: 200,   // fixed point (x10) => 20
      y: 50,    // fixed point (x10) => 5
      vx: 5,
      vy: 0,
      fuel: 500,
      score: 0,
      status: 0, // 0: PLAYING, 1: LANDED, 2: CRASHED
      padX: 50,
      padY: 60,
      padW: 28,
      frameSeq: 0,
    };

    this.inputState = 0;
    this.channel = 1;
    this.totalFrames = 120;
    this.mgmtFrames = 80;
    this.ctrlFrames = 25;
    this.dataFrames = 15;
    this.avgRssi = -64;
  }

  start() {
    this.client.isConnected = true;
    this.client.isConnecting = false;

    // Send Simulated Device Info
    this.client.deviceInfo = {
      product: 'SliverOS [SIMULATED]',
      version: '1.0.0-sim',
      buildId: 'BUILD-SIM-REV1',
      gitRevision: 'SIMULATOR-20260925',
      chipFamily: 'ESP32-S3 (SIM)',
      chipRevision: 1,
      flashSizeBytes: 8 * 1024 * 1024,
      psramSizeBytes: 8 * 1024 * 1024,
      internalSramTotal: 512 * 1024,
      internalSramFree: 384 * 1024,
      psramTotal: 8 * 1024 * 1024,
      psramFree: 8 * 1024 * 1024,
      isSimulated: true,
    };

    this.client._emit('connectionStateChange', 'simulated');
    this.client._emit('hello', { ready: true, isSimulated: true });
    this.client._emit('deviceInfo', this.client.deviceInfo);
    this.client._emit('log', {
      level: 1,
      timestampMs: Date.now(),
      text: '[SIMULATOR] SliverOS simulated device initialized in browser environment.',
    });

    this._sendInitialStatus();

    // Game loop tick at 20 Hz (50 ms)
    this.gameTimer = setInterval(() => this._gameTick(), 50);

    // General telemetry tick at 2 Hz (500 ms)
    this.timer = setInterval(() => this._telemetryTick(), 500);
  }

  stop() {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
    if (this.gameTimer) {
      clearInterval(this.gameTimer);
      this.gameTimer = null;
    }
    this.client.isConnected = false;
    this.client._emit('connectionStateChange', 'disconnected');
  }

  setInput(mask) {
    this.inputState = mask;
  }

  launchApp(appId) {
    this.activeApp = appId;
    for (const app of this.appStates) {
      app.state = (app.id === appId) ? 3 : 2; // 3 = ACTIVE, 2 = READY
    }
    this.client._emit('appState', {
      appId,
      state: 3,
      runs: ++this.appStates[appId].runs,
      errors: 0,
      data: new Uint8Array(0),
    });
    this._sendInitialStatus();
  }

  exitApp(appId) {
    if (this.appStates[appId]) {
      this.appStates[appId].state = 2; // READY
    }
    this.client._emit('appState', {
      appId,
      state: 2,
      runs: this.appStates[appId].runs,
      errors: 0,
      data: new Uint8Array(0),
    });
  }

  resetGame() {
    this.lander.x = 200;
    this.lander.y = 50;
    this.lander.vx = 5;
    this.lander.vy = 0;
    this.lander.fuel = 500;
    this.lander.status = 0;
    this.lander.score = 0;
  }

  handleTerminal(command) {
    const trimmed = command.trim();
    const parts = trimmed.split(' ');
    const cmd = parts[0].toLowerCase();

    switch (cmd) {
      case 'help':
        this.client._emit('terminalOutput',
          'SliverOS Shell Commands [SIMULATOR]:\r\n' +
          '  help       - Print this command list\r\n' +
          '  apps       - List 4 registered applications & states\r\n' +
          '  status     - Show uptime, tick & scheduler info\r\n' +
          '  mem        - Display SRAM arena & PSRAM memory metrics\r\n' +
          '  tasks      - List 8 TCBs and CPU runtime statistics\r\n' +
          '  vfs        - Display VFS sector status & record count\r\n' +
          '  version    - Display OS product & build identification\r\n' +
          '  clear      - Clear terminal window\r\n' +
          '  reboot     - Restart simulated executive\r\n'
        );
        break;

      case 'apps':
        this.client._emit('terminalOutput',
          'SliverOS Applications (4 Core Apps) [SIMULATED]:\r\n' +
          `  [0] BLE_HID     State: ${this.appStates[0].state === 3 ? 'ACTIVE' : 'READY'} Runs: ${this.appStates[0].runs}\r\n` +
          `  [1] WIFI_DIAG   State: ${this.appStates[1].state === 3 ? 'ACTIVE' : 'READY'} Runs: ${this.appStates[1].runs}\r\n` +
          `  [2] NET_DIAG    State: ${this.appStates[2].state === 3 ? 'ACTIVE' : 'READY'} Runs: ${this.appStates[2].runs}\r\n` +
          `  [3] RETRO_GAMES State: ${this.appStates[3].state === 3 ? 'ACTIVE' : 'READY'} Runs: ${this.appStates[3].runs}\r\n`
        );
        break;

      case 'status':
        this.client._emit('terminalOutput',
          `Kernel State: RUNNING [SIM] | Tick: ${this.tick} | Uptime: ${Math.floor(this.tick / 1000)} s | Iterations: ${this.tick * 2}\r\n`
        );
        break;

      case 'mem':
        this.client._emit('terminalOutput',
          'Internal SRAM Arena (128 KB) [SIMULATED]:\r\n' +
          '  Capacity: 131072 B | Used: 24576 B | Free: 106496 B | Peak: 36864 B\r\n' +
          '  Allocs: 84 | Frees: 62 | Failures: 0\r\n' +
          'External PSRAM (8 MB):\r\n' +
          '  Total: 8388608 B | Free: 8257536 B\r\n'
        );
        break;

      case 'tasks':
        this.client._emit('terminalOutput',
          'ID State Prio Period Runs   Last(us) Worst(us) Overruns Name\r\n' +
          ' 0 RUN     0      1   4820       120       450        0 host_proto\r\n' +
          ' 1 SLEEP   2      1   1240        85       210        0 ble_hid\r\n' +
          ' 2 SLEEP   4      5    860       340       890        0 wifi_diag\r\n' +
          ' 3 SLEEP   6     10    420       510      1100        0 net_diag\r\n' +
          ' 4 READY   7      2   3120       420       780        0 retro_games\r\n'
        );
        break;

      case 'vfs':
        this.client._emit('terminalOutput',
          'VFS Sector Storage (NOR Flash \'osfs\' Partition) [SIMULATED]:\r\n' +
          '  Total Sectors: 672 (4 KB each = 2688 KB total)\r\n' +
          '  Power-loss Safe Records: Active & Verified with CRC32\r\n'
        );
        break;

      case 'version':
        this.client._emit('terminalOutput',
          'SliverOS Embedded Executive v1.0.0-sim (Build SIM-BROWSER-2026)\r\n' +
          'Target Hardware: 7Semi ESP32-S3 (8MB Flash, 8MB PSRAM) [SIMULATED]\r\n'
        );
        break;

      case 'clear':
        this.client._emit('terminalOutput', '\x1b[2J\x1b[H');
        break;

      case 'reboot':
        this.client._emit('terminalOutput', 'Restarting simulated executive...\r\n');
        setTimeout(() => {
          this.tick = 0;
          this.client._emit('log', { level: 1, timestampMs: Date.now(), text: '[SIMULATOR] Warm reboot completed.' });
        }, 500);
        break;

      default:
        this.client._emit('terminalOutput', `Unknown command: '${cmd}'. Type 'help' for command list.\r\n`);
        break;
    }
  }

  _sendInitialStatus() {
    this._telemetryTick();
  }

  _gameTick() {
    if (this.activeApp !== 3) return;

    this.lander.frameSeq++;

    if (this.lander.status === 0) { // PLAYING
      // Gravity
      this.lander.vy += 1;

      // Controls (from inputState mask)
      // Bit 0: UP, Bit 2: LEFT, Bit 3: RIGHT, Bit 4: A, Bit 5: B
      if (this.lander.fuel > 0) {
        if ((this.inputState & 0x01) || (this.inputState & 0x10)) { // UP / A (thrust)
          this.lander.vy -= 2;
          this.lander.fuel = Math.max(0, this.lander.fuel - 1);
        }
        if (this.inputState & 0x04) { // LEFT
          this.lander.vx -= 1;
          this.lander.fuel = Math.max(0, this.lander.fuel - 1);
        }
        if (this.inputState & 0x08) { // RIGHT
          this.lander.vx += 1;
          this.lander.fuel = Math.max(0, this.lander.fuel - 1);
        }
      }

      // Update positions
      this.lander.x += this.lander.vx;
      this.lander.y += this.lander.vy;

      const px = Math.floor(this.lander.x / 10);
      const py = Math.floor(this.lander.y / 10);

      // Check boundary
      if (py >= this.lander.padY - 4) {
        const withinPad = (px >= this.lander.padX && (px + 6) <= (this.lander.padX + this.lander.padW));
        const safeVelocity = (this.lander.vy < 8 && Math.abs(this.lander.vx) < 4);

        if (withinPad && safeVelocity) {
          this.lander.status = 1; // LANDED
          this.lander.score += 100 + this.lander.fuel;
        } else {
          this.lander.status = 2; // CRASHED
        }
      }
    } else {
      if (this.inputState & 0x10 || this.inputState & 0x20) { // Button A or B to reset
        this.resetGame();
      }
    }

    this.client._emit('gameState', {
      x: this.lander.x / 10,
      y: this.lander.y / 10,
      vx: this.lander.vx,
      vy: this.lander.vy,
      fuel: this.lander.fuel,
      score: this.lander.score,
      status: this.lander.status,
      padX: this.lander.padX,
      padY: this.lander.padY,
      padW: this.lander.padW,
      frameSeq: this.lander.frameSeq,
    });
  }

  _telemetryTick() {
    this.tick += 500;

    // Simulate passive Wi-Fi updates
    if (this.activeApp === 1) {
      this.totalFrames += Math.floor(Math.random() * 8) + 1;
      this.mgmtFrames += Math.floor(Math.random() * 5);
      this.ctrlFrames += Math.floor(Math.random() * 2);
      this.dataFrames += Math.floor(Math.random() * 3);
      this.channel = (this.channel % 11) + 1;
      this.avgRssi = -55 - Math.floor(Math.random() * 20);

      this.client._emit('appState', {
        appId: 1,
        state: 3,
        runs: this.appStates[1].runs,
        errors: 0,
        wifiStats: {
          channel: this.channel,
          totalFrames: this.totalFrames,
          mgmtFrames: this.mgmtFrames,
          ctrlFrames: this.ctrlFrames,
          dataFrames: this.dataFrames,
          avgRssi: this.avgRssi,
        }
      });
    }

    // Device Status
    this.client._emit('deviceStatus', {
      tick: this.tick,
      uptimeSeconds: Math.floor(this.tick / 1000),
      schedulerIterations: this.tick * 2,
      activeApp: this.activeApp,
      kernelState: 3, // RUNNING
      taskCount: 5,
      faultCount: 0,
      tasks: [
        { id: 0, state: 2, priority: 0, periodTicks: 1, executionCount: this.tick, lastExecutionUs: 120, worstExecutionUs: 450, overrunCount: 0, name: 'host_proto' },
        { id: 1, state: 4, priority: 2, periodTicks: 1, executionCount: Math.floor(this.tick / 2), lastExecutionUs: 85, worstExecutionUs: 210, overrunCount: 0, name: 'ble_hid' },
        { id: 2, state: 4, priority: 4, periodTicks: 5, executionCount: Math.floor(this.tick / 5), lastExecutionUs: 340, worstExecutionUs: 890, overrunCount: 0, name: 'wifi_diag' },
        { id: 3, state: 4, priority: 6, periodTicks: 10, executionCount: Math.floor(this.tick / 10), lastExecutionUs: 510, worstExecutionUs: 1100, overrunCount: 0, name: 'net_diag' },
        { id: 4, state: 1, priority: 7, periodTicks: 2, executionCount: Math.floor(this.tick / 2), lastExecutionUs: 420, worstExecutionUs: 780, overrunCount: 0, name: 'retro_games' },
      ]
    });

    // Memory Status
    this.client._emit('memoryStatus', {
      arenaCapacity: 131072,
      arenaUsed: 24576 + (Math.sin(this.tick / 5000) * 4096),
      arenaFree: 106496,
      arenaPeak: 36864,
      allocationCount: 84,
      freeCount: 62,
      failedAllocations: 0,
      psramTotal: 8 * 1024 * 1024,
      psramFree: 8257536,
    });

    // Storage Status
    this.client._emit('storageStatus', {
      totalSectors: 672,
      freeSectors: 670,
      activeSectors: 1,
      obsoleteSectors: 1,
      recordCommitCount: 16,
    });
  }
}
