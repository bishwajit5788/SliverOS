# Web Serial Flashing Guide

This document explains how to flash MicroKernel OS directly from a Chromium-based browser to an ESP32 development board.

---

## 1. Hardware Setup

1. Connect your ESP32 board to your computer using a **micro-USB or USB-C data cable**.
   > [!CAUTION]
   > Ensure your USB cable has internal data wires (D+/D-). Many consumer cables are power/charge-only and will not present a USB serial device to your operating system.
2. Confirm the USB UART bridge driver is recognized:
   - macOS: `/dev/cu.usbserial-*` or `/dev/cu.wchusbserial*`
   - Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
   - Windows: `COM3`, `COM4`, etc.

---

## 2. In-Browser Flashing Workflow

1. Open the Web Serial Flasher in Google Chrome, Microsoft Edge, or Opera:
   ```bash
   cd flasher && npm run dev
   ```
   Navigate to `http://localhost:3000`.

2. Click **CONNECT DEVICE**.
   - A browser permission dialog will appear listing available USB serial devices.
   - Select your ESP32 board and click **Connect**.

3. Automatic Detection & Pre-Flight Checks:
   - The flasher engages the auto-reset circuit (toggling DTR/RTS) to place the chip into ROM bootloader mode.
   - Sends `ESP_CMD_SYNC` packets until synchronized.
   - Reads hardware registers to detect chip family (`ESP32`, `ESP32-C3`, or `ESP32-S3`) and revision.
   - Verifies flash size and checks target compatibility against `releases.json`.
   - Pre-verifies SHA-256 cryptographic hashes for all firmware images.

4. Click **INSTALL MICROKERNEL OS**:
   - The flasher erases target flash sectors (`ESP_CMD_FLASH_BEGIN`).
   - Streams 4KB blocks with XOR checksums (`ESP_CMD_FLASH_DATA`).
   - Displays real-time speed (KB/s), transferred bytes, ETA, and per-image progress bars.
   - Queries hardware-computed MD5 checksums (`ESP_CMD_SPI_FLASH_MD5`) from the ROM bootloader to verify flash integrity.

5. Automatic Reboot:
   - The flasher pulses RTS to reset the chip into user application mode.
   - The board boots MicroKernel OS and outputs the diagnostic banner to the serial monitor.

---

## 3. Manual Boot Mode Fallback

If auto-reset fails to enter the ROM bootloader (e.g. on boards lacking RTS/DTR transistor circuits):
1. Hold down the **BOOT** (or **IO0**) button on the ESP32 board.
2. Momentarily press the **EN** (or **RST**) button.
3. Release the **BOOT** button.
4. Click **CONNECT DEVICE** in the browser flasher.

---

## 4. Recommended Link Speeds

- **115,200 baud**: Highest reliability; recommended for noisy USB hubs or long cables.
- **460,800 baud**: Default standard speed; full 1.5MB firmware flash completes in ~12 seconds.
- **921,600 baud**: Ultra-fast; supported by CP2102 and CH9102 bridges.
