# SliverOS Firmware Flashing Guide

This document details the flashing procedure for the **7Semi ESP32-S3-Dev-BoardC-1U-N8R8** using the built-in Web Serial flasher or standard ESP-IDF tooling.

---

## 1. Target Hardware & Physical Connection

- **Target Board**: **7Semi ESP32-S3-Dev-BoardC-1U-N8R8**
- **Silicon**: ESP32-S3-WROOM-1 MCN8R8 (8 MB Flash, 8 MB Octal PSRAM)
- **USB Interface**: Native USB Serial/JTAG Controller connected via onboard USB-C port
- **Host System**: Mac running macOS (Apple Silicon or Intel)
- **Known Working Port**: `/dev/cu.usbmodem101`

> [!CAUTION]
> Always use a certified USB-C data cable. Charge-only cables lack D+/D- signal lines and will not enumerate the Native USB device on macOS.

---

## 2. Browser Flashing via SliverOS Web Host

The Web Host includes an integrated, official Espressif ROM bootloader flasher accessible from the sidebar **Flasher** tab:

1. **Connect Hardware**: Plug the 7Semi ESP32-S3 into your Mac using the USB-C cable.
2. **Open Web Host**: Open the SliverOS Web Host in Google Chrome, Microsoft Edge, or a Chromium-based browser.
3. **Navigate to Flasher**: Click **Flasher** in the sidebar.
4. **Trigger Flashing**: Click **INSTALL SLIVEROS FIRMWARE**.
5. **Grant Port Permission**: When the browser device picker appears, select the `ESP32-S3` or `USB JTAG/serial debug unit`.
6. **Automatic Bootloader Handshake & Target Validation**:
   - The flasher asserts DTR/RTS auto-reset signals to enter ROM bootloader mode.
   - Synchronizes with the silicon ROM over SLIP framing.
   - Reads hardware registers: confirms `ESP32-S3` revision.
   - **Target Rejection**: If an incompatible chip (such as ESP32 Classic or ESP32-C3) is detected, the flasher immediately aborts to prevent corrupting the target device.
7. **Integrity Verification**:
   - Downloads the firmware binaries defined in `releases.json`.
   - Rejects test mock fixtures labeled with test banners.
   - Computes SHA-256 digests over the downloaded binaries and validates against manifest signatures.
8. **Flash Writing**:
   - Erases required sectors on the 8 MB flash.
   - Writes the bootloader to offset `0x0000`.
   - Writes the partition table to offset `0x8000`.
   - Writes the SliverOS application binary to offset `0x20000` (`ota_0`).
   - Verifies written blocks using on-chip hardware MD5 queries.
9. **Automatic Reset**:
   - Resets the ESP32-S3 into application mode.
   - The chip boots SliverOS and begins streaming SLVR/1 protocol telemetry.

---

## 3. Flash Memory Offsets Summary (8 MB Flash)

| Component | Flash Offset | Partition | Size Allocation |
|---|---|---|---|
| First-Stage Bootloader | `0x0000` | N/A (ROM reserved) | ~28 KiB |
| Partition Table | `0x8000` | N/A | 3 KiB (0x1000 aligned) |
| NVS Storage | `0x9000` | `nvs` | 24 KiB (`0x6000`) |
| OTA Data | `0xF000` | `otadata` | 8 KiB (`0x2000`) |
| PHY Initialization | `0x11000` | `phy_init` | 4 KiB (`0x1000`) |
| **SliverOS Application** | **`0x20000`** | `ota_0` | **2.5 MiB (`0x280000`)** |
| Backup OTA Slot | `0x2A0000` | `ota_1` | 2.5 MiB (`0x280000`) |
| SliverOS File System | `0x520000` | `osfs` | 2.625 MiB (`0x2A0000`) |

---

## 4. Integrity vs. Authenticity

- **Integrity Verification**: The Web Host computes SHA-256 digests over all firmware images before flashing to guarantee that files were downloaded without transmission corruption or truncation.
- **Authenticity (Cryptographic Signing)**: SHA-256 alone does not provide authenticity against malicious actors. ESP32-S3 hardware Secure Boot v2 and RSA-3072 / ECDSA signing can be enabled on production units to cryptographically lock execution to authorized keys.

---

## 5. Command-Line Flashing Fallback (ESP-IDF)

If flashing via CLI is preferred:
```bash
# Flash entire system using esptool.py
idf.py -p /dev/cu.usbmodem101 flash monitor
```
Or directly via `esptool.py`:
```bash
esptool.py --chip esp32s3 -p /dev/cu.usbmodem101 -b 460800 \
    --before default_reset --after hard_reset write_flash -z \
    --flash_mode dio --flash_freq 80m --flash_size 8MB \
    0x0000 build/bootloader/bootloader.bin \
    0x8000 build/partition_table/partition-table.bin \
    0x20000 build/microkernel-esp32.bin
```
