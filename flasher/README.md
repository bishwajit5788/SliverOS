# MicroKernel OS Web Serial Flasher

A modern, browser-based flashing application for installing the MicroKernel OS directly onto ESP32, ESP32-C3, and ESP32-S3 boards without installing local Python or `esptool`.

## Prerequisites

- **Supported Browsers**: Google Chrome, Microsoft Edge, Opera, or Chromium-based browsers with Web Serial enabled.
- **Hardware**: An ESP32 development board connected to your laptop via a **USB data cable** (ensure the cable is not charge-only).
- **USB Serial Drivers**: Silicon Labs CP210x, WCH CH340, FTDI, or native Espressif USB JTAG/Serial drivers.

## Getting Started

1. Install dependencies:
   ```bash
   npm install
   ```

2. Start local development server:
   ```bash
   npm run dev
   ```
   Open `http://localhost:3000` in Google Chrome or Edge.

3. Build production distribution bundle:
   ```bash
   npm run build
   ```

4. Run unit tests:
   ```bash
   npm test
   ```

## Architecture & Flash Flow

1. **Port Acquisition**: Uses `navigator.serial.requestPort()` with Vendor ID filters.
2. **Auto-Reset Sequence**: Toggles DTR and RTS signals to place ESP32 into ROM bootloader mode (EN low, IO0 low).
3. **ROM Synchronization**: Transmits 36-byte 0x07 sync packets (`ESP_CMD_SYNC`) at chosen baud rate.
4. **Chip Identification**: Reads hardware registers (`0x3FF00044`, `0x60007000`) to detect silicon family (ESP32 / ESP32-C3 / ESP32-S3).
5. **Pre-Flash Safety**:
   - Matches detected silicon with firmware release manifest (`releases.json`).
   - Verifies browser-side SHA-256 integrity using the Web Crypto API.
6. **Hardware Flashing**:
   - `ESP_CMD_FLASH_BEGIN`: Erases target sectors on flash.
   - `ESP_CMD_FLASH_DATA`: Transmits 4KB chunks with XOR checksums and sequence indices.
   - `ESP_CMD_FLASH_END`: Finalizes image.
7. **Post-Flash Verification**: Invokes `ESP_CMD_SPI_FLASH_MD5` to compute and match hardware MD5 on the flash sectors.
8. **Hard Reset**: Pulses RTS to reboot into MicroKernel OS executive.
