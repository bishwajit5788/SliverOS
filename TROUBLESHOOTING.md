# SliverOS Hardware & System Troubleshooting Guide

This guide addresses common hardware connection, Web Serial, protocol framing, and runtime faults when running SliverOS on the **7Semi ESP32-S3-Dev-BoardC-1U-N8R8**.

---

## 1. Physical Hardware & USB Ports (7Semi Board)

The 7Semi ESP32-S3 development board features **dual USB-C ports**:
1. **USB Port (Native USB Serial/JTAG)**:
   - Connected directly to ESP32-S3 internal USB D- (GPIO 19) and D+ (GPIO 20).
   - Enumerate on macOS as: `/dev/cu.usbmodem*` (e.g., `/dev/cu.usbmodem101`).
   - **This is the primary port for SliverOS runtime communication and the Web Host desktop interface.**
2. **UART Port (CP2102N Bridge)**:
   - Connected to CP2102N USB-UART bridge (GPIO 43 TX / GPIO 44 RX).
   - Enumerates on macOS as: `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART*`.
   - Used as secondary hardware UART if needed.

> [!TIP]
> Ensure your USB-C cable is connected to the **Native USB port** (enumerating as `/dev/cu.usbmodem*`) for SliverOS Web Host desktop interaction.

---

## 2. Web Serial Port & Browser Issues

### "Web Serial API is not supported in this browser"
- **Cause**: Apple Safari and Mozilla Firefox do not implement the Web Serial standard.
- **Resolution**: Open the SliverOS Web Host in **Google Chrome**, **Microsoft Edge**, **Opera**, or **Brave** (Chromium v89+).

### No devices appear in the Web Serial picker dialog
- **Check Cable**: Ensure the USB-C cable has internal data conductors. Charge-only cables will not enumerate any device.
- **Check macOS Port Status**: Run in terminal:
  ```bash
  ls -la /dev/cu.usb*
  ```
  If `/dev/cu.usbmodem101` appears, the device is properly enumerated by macOS.
- **USB Hubs**: Unplug from unpowered USB hubs or adapters and plug directly into the Mac's Thunderbolt/USB-C port.

---

## 3. Protocol Handshake & Web Host Connection

### "Handshake timed out waiting for HELLO frame"
- **Cause**: The ESP32 is either in bootloader mode, not running SliverOS, or the serial port is busy in another terminal.
- **Resolution**:
  1. Ensure no other applications (e.g. `screen`, `minicom`, Arduino IDE, or `idf.py monitor`) are holding `/dev/cu.usbmodem101` open.
  2. Press the onboard **RST** button to reboot into application mode.
  3. Click **CONNECT ESP32-S3** in the Web Host interface.

### "Corrupted CRC / Malformed Frame Rejected"
- **Cause**: Serial line noise or mismatched baud rate.
- **Resolution**: The SLVR/1 parser automatically drops noisy bytes and resynchronizes on the next `'S'` magic sequence without crashing. If noise persists, verify cable integrity and ensure baud rate is set to 115200.

---

## 4. Bootloader Entry & Flashing Issues

### Flasher fails to auto-reset into ROM Bootloader
- **Manual Boot Sequence**:
  1. Hold down the **BOOT** button on the 7Semi board.
  2. Momentarily press and release the **RST** button.
  3. Release the **BOOT** button.
  4. Click **INSTALL SLIVEROS FIRMWARE** in the browser flasher.

### "Target chip rejected: Flashing aborted"
- **Cause**: A non-ESP32-S3 device (such as ESP32 Classic or ESP32-C3) was selected.
- **Resolution**: SliverOS strictly targets ESP32-S3. Connect the confirmed 7Semi ESP32-S3-Dev-BoardC-1U-N8R8.

---

## 5. Runtime Kernel & Cooperative Scheduler Faults

### `MK_FAULT_TASK_OVERRUN` logged to Console
- **Cause**: A cooperative task took longer than its allocated slice (e.g., >10ms for BLE HID).
- **Resolution**: Tasks must yield cooperatively. Ensure no blocking functions (`vTaskDelay`, synchronous socket waits, long polling) are called within application dispatch routines.

### `MK_FAULT_MEMORY_ARENA_EXHAUSTED`
- **Cause**: Allocations exceeded the 128 KiB internal SRAM arena.
- **Resolution**: Check buffer allocations in the Developer Terminal using the `mem` command. Ensure transient buffers are freed via `my_free()`.

### Hardware Watchdog (TWDT) Reset
- **Cause**: A task locked up the executive thread for more than 5.0 seconds.
- **Resolution**: FreeRTOS hardware watchdog resets the silicon. Review recent terminal and fault logs to identify which task blocked arbitration.
