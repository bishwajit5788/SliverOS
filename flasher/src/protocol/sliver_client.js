/**
 * sliver_client.js
 * High-level Web Serial client for SliverOS runtime communication.
 * Manages device picker, port opening, SLVR/1 handshake, events, and clean disconnects.
 */

import {
  MsgType,
  encodeFrame,
  SlvrStreamParser,
  unpackDeviceInfo,
  unpackDeviceStatus,
  unpackMemoryStatus,
  unpackStorageStatus,
  unpackGameState,
  unpackLog,
  unpackTerminal,
} from './host_protocol.js';

export class SliverClient {
  constructor(logger = console.log) {
    this.log = logger;
    this.port = null;
    this.reader = null;
    this.writer = null;
    this.parser = new SlvrStreamParser(this._onFrame.bind(this));
    this.listeners = new Map();
    this.isConnected = false;
    this.isConnecting = false;
    this.deviceInfo = null;
    this.sequence = 0;
    this.keepAliveTimer = null;
  }

  on(event, callback) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event).push(callback);
    return () => {
      const arr = this.listeners.get(event);
      if (arr) {
        const idx = arr.indexOf(callback);
        if (idx !== -1) arr.splice(idx, 1);
      }
    };
  }

  _emit(event, data) {
    const arr = this.listeners.get(event);
    if (arr) {
      for (const cb of arr) {
        try {
          cb(data);
        } catch (e) {
          console.error(`Error in event listener for ${event}:`, e);
        }
      }
    }
  }

  /**
   * Prompts user with Web Serial device picker and connects
   */
  async requestDeviceAndConnect(baudRate = 115200) {
    if (!navigator.serial) {
      throw new Error('Web Serial API is not supported in this browser. Please use Chrome, Edge, or a Chromium browser on macOS.');
    }

    if (this.isConnected || this.isConnecting) {
      return;
    }

    this.isConnecting = true;
    this._emit('connectionStateChange', 'connecting');

    try {
      this.port = await navigator.serial.requestPort({
        filters: [
          // ESP32-S3 USB Serial / JTAG controller VID: 0x303A, PID: 0x1001
          { usbVendorId: 0x303A, usbProductId: 0x1001 },
          // Silicon Labs CP2102 USB to UART bridge VID: 0x10C4, PID: 0xEA60
          { usbVendorId: 0x10C4, usbProductId: 0xEA60 },
          // Espressif USB CDC / OTG
          { usbVendorId: 0x303A },
        ]
      });

      await this.port.open({
        baudRate: baudRate,
        dataBits: 8,
        stopBits: 1,
        parity: 'none',
        flowControl: 'none',
        bufferSize: 8192,
      });

      this.writer = this.port.writable.getWriter();
      this.parser.reset();

      // Setup disconnect listener from Web Serial
      if (this.port.addEventListener) {
        this.port.addEventListener('disconnect', () => {
          this.log('[INFO] USB Cable disconnected by host.');
          this.disconnect();
        });
      }

      // Start read loop in background
      this._startReadLoop();

      // Send CONNECT command to initiate handshake
      await this.sendCommand(MsgType.CMD_CONNECT, null);

      // Wait up to 3 seconds for HELLO / DEVICE_INFO response
      const handshakeDone = await this._waitForHandshake(3000);
      if (!handshakeDone) {
        // Fallback: If device was rebooting or printing ASCII boot log, query info again
        await this.sendCommand(MsgType.CMD_GET_INFO, null);
        await new Promise(r => setTimeout(r, 400));
      }

      this.isConnected = true;
      this.isConnecting = false;
      this._emit('connectionStateChange', 'connected');
      this.log('[SUCCESS] SliverOS Host Protocol (SLVR/1) connection established.');

      // Start periodic 1-second ping / status refresh
      this.keepAliveTimer = setInterval(() => {
        if (this.isConnected) {
          this.sendCommand(MsgType.CMD_GET_STATUS, null).catch(() => {});
        }
      }, 1000);

    } catch (err) {
      this.isConnecting = false;
      this._emit('connectionStateChange', 'disconnected');
      await this.disconnect();
      throw err;
    }
  }

  async _waitForHandshake(timeoutMs) {
    const start = Date.now();
    while (Date.now() - start < timeoutMs) {
      if (this.deviceInfo) {
        return true;
      }
      await new Promise(r => setTimeout(r, 50));
    }
    return false;
  }

  async _startReadLoop() {
    try {
      while (this.port && this.port.readable) {
        this.reader = this.port.readable.getReader();
        try {
          while (true) {
            const { value, done } = await this.reader.read();
            if (done) {
              break;
            }
            if (value) {
              this.parser.feed(value);
            }
          }
        } catch (readErr) {
          if (this.isConnected) {
            this.log('[WARN] Serial stream read error: ' + readErr.message);
          }
        } finally {
          this.reader.releaseLock();
          this.reader = null;
        }
      }
    } catch (err) {
      this.log('[INFO] Port read loop finished: ' + err.message);
    } finally {
      if (this.isConnected) {
        this.disconnect();
      }
    }
  }

  async sendCommand(type, payload = null) {
    if (!this.writer) {
      return;
    }

    const frame = {
      type,
      flags: 0,
      sequence: this.sequence++ & 0xFF,
      payload: payload ? (payload instanceof Uint8Array ? payload : new Uint8Array(payload)) : null,
    };

    const wireBytes = encodeFrame(frame);
    await this.writer.write(wireBytes);
  }

  async launchApp(appId) {
    await this.sendCommand(MsgType.CMD_LAUNCH_APP, new Uint8Array([appId]));
  }

  async exitApp(appId) {
    await this.sendCommand(MsgType.CMD_EXIT_APP, new Uint8Array([appId]));
  }

  async sendAppInput(appId, inputMask) {
    await this.sendCommand(MsgType.CMD_APP_INPUT, new Uint8Array([appId, inputMask]));
  }

  async sendTerminalCommand(cmdString) {
    const enc = new TextEncoder().encode(cmdString.trim());
    await this.sendCommand(MsgType.CMD_TERMINAL_INPUT, enc);
  }

  async resetDevice() {
    await this.sendCommand(MsgType.CMD_RESET, null);
  }

  async disconnect() {
    if (this.keepAliveTimer) {
      clearInterval(this.keepAliveTimer);
      this.keepAliveTimer = null;
    }

    this.isConnected = false;
    this.isConnecting = false;
    this.deviceInfo = null;

    try {
      if (this.reader) {
        await this.reader.cancel();
        this.reader = null;
      }
    } catch (e) {}

    try {
      if (this.writer) {
        await this.writer.close();
        this.writer = null;
      }
    } catch (e) {}

    try {
      if (this.port) {
        await this.port.close();
        this.port = null;
      }
    } catch (e) {}

    this._emit('connectionStateChange', 'disconnected');
  }

  _onFrame(frame) {
    switch (frame.type) {
      case MsgType.HELLO:
        this._emit('hello', { ready: true });
        break;

      case MsgType.DEVICE_INFO:
        this.deviceInfo = unpackDeviceInfo(frame.payload);
        this._emit('deviceInfo', this.deviceInfo);
        break;

      case MsgType.DEVICE_STATUS: {
        const status = unpackDeviceStatus(frame.payload);
        this._emit('deviceStatus', status);
        break;
      }

      case MsgType.MEMORY_STATUS: {
        const mem = unpackMemoryStatus(frame.payload);
        this._emit('memoryStatus', mem);
        break;
      }

      case MsgType.STORAGE_STATUS: {
        const storage = unpackStorageStatus(frame.payload);
        this._emit('storageStatus', storage);
        break;
      }

      case MsgType.GAME_STATE: {
        const game = unpackGameState(frame.payload);
        this._emit('gameState', game);
        break;
      }

      case MsgType.APP_STATE: {
        this._emit('appState', {
          appId: frame.payload[0],
          state: frame.payload[1],
          runs: (frame.payload[2] << 8) | frame.payload[3],
          errors: (frame.payload[4] << 8) | frame.payload[5],
          data: frame.payload.slice(6),
        });
        break;
      }

      case MsgType.LOG: {
        const log = unpackLog(frame.payload);
        this._emit('log', log);
        break;
      }

      case MsgType.TERMINAL_OUTPUT: {
        const term = unpackTerminal(frame.payload);
        this._emit('terminalOutput', term.text);
        break;
      }

      case MsgType.PONG:
        this._emit('pong', frame.payload);
        break;

      default:
        break;
    }
  }
}
