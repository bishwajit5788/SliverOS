/**
 * webserial.js
 * Hardware Web Serial API driver with stream buffer, timeout guards, and signal control.
 */

export class WebSerialPort {
  constructor() {
    this.port = null;
    this.reader = null;
    this.writer = null;
    this.isOpen = false;
    this.rxBuffer = [];
    this.disconnectCallback = null;
  }

  isSupported() {
    return 'serial' in navigator;
  }

  onDisconnect(cb) {
    this.disconnectCallback = cb;
    if (this.isSupported()) {
      navigator.serial.addEventListener('disconnect', (event) => {
        if (this.port && event.target === this.port) {
          this.isOpen = false;
          if (this.disconnectCallback) {
            this.disconnectCallback(new Error('USB cable disconnected / device unplugged.'));
          }
        }
      });
    }
  }

  async requestPort() {
    if (!this.isSupported()) {
      throw new Error('Web Serial API is not supported in this browser. Please use Chrome, Edge, or Opera.');
    }
    this.port = await navigator.serial.requestPort({
      filters: [
        { usbVendorId: 0x10C4 }, // Silicon Labs CP210x
        { usbVendorId: 0x1A86 }, // WCH CH340 / CH9102
        { usbVendorId: 0x0403 }, // FTDI
        { usbVendorId: 0x303A }  // Espressif USB JTAG / Serial
      ]
    });
    return this.port;
  }

  async open(baudRate = 115200) {
    if (!this.port) {
      throw new Error('No serial port selected.');
    }

    await this.port.open({
      baudRate,
      dataBits: 8,
      stopBits: 1,
      parity: 'none',
      bufferSize: 16384,
      flowControl: 'none'
    });

    this.reader = this.port.readable.getReader();
    this.writer = this.port.writable.getWriter();
    this.isOpen = true;
    this.rxBuffer = [];

    // Background reader loop
    this.startReadLoop();
  }

  async startReadLoop() {
    try {
      while (this.isOpen && this.reader) {
        const { value, done } = await this.reader.read();
        if (done) {
          break;
        }
        if (value) {
          for (let i = 0; i < value.length; i++) {
            this.rxBuffer.push(value[i]);
          }
        }
      }
    } catch (err) {
      if (this.isOpen) {
        this.isOpen = false;
        if (this.disconnectCallback) {
          this.disconnectCallback(err);
        }
      }
    }
  }

  async setSignals(signals) {
    if (!this.port) return;
    try {
      await this.port.setSignals(signals);
    } catch (e) {
      // Some serial drivers don't support signal control
      console.warn('SetSignals notice:', e);
    }
  }

  async write(data) {
    if (!this.writer || !this.isOpen) {
      throw new Error('Serial port is not writable.');
    }
    await this.writer.write(data);
  }

  async readUntil(endByte, timeoutMs = 2000) {
    const startTime = Date.now();

    while (Date.now() - startTime < timeoutMs) {
      if (!this.isOpen) {
        throw new Error('Port disconnected during read.');
      }

      const idx = this.rxBuffer.indexOf(endByte);
      if (idx !== -1) {
        const packet = this.rxBuffer.splice(0, idx + 1);
        return new Uint8Array(packet);
      }

      await new Promise(r => setTimeout(r, 10));
    }

    throw new Error(`Read timeout (${timeoutMs}ms) waiting for byte 0x${endByte.toString(16)}`);
  }

  async readBytes(count, timeoutMs = 2000) {
    const startTime = Date.now();

    while (Date.now() - startTime < timeoutMs) {
      if (!this.isOpen) {
        throw new Error('Port disconnected during read.');
      }

      if (this.rxBuffer.length >= count) {
        const packet = this.rxBuffer.splice(0, count);
        return new Uint8Array(packet);
      }

      await new Promise(r => setTimeout(r, 10));
    }

    throw new Error(`Read timeout waiting for ${count} bytes`);
  }

  drain() {
    this.rxBuffer = [];
  }

  async close() {
    this.isOpen = false;
    if (this.reader) {
      try {
        await this.reader.cancel();
      } catch (e) {
        console.warn('Reader cancel warning:', e);
      }
      this.reader.releaseLock();
      this.reader = null;
    }
    if (this.writer) {
      try {
        await this.writer.close();
      } catch (e) {
        console.warn('Writer close warning:', e);
      }
      this.writer.releaseLock();
      this.writer = null;
    }
    if (this.port) {
      try {
        await this.port.close();
      } catch (e) {
        console.warn('Port close warning:', e);
      }
      this.port = null;
    }
  }
}
