# SliverOS

A **Custom Cooperative Embedded OS Runtime Executive for ESP32-S3 built on ESP-IDF**, paired with a **Vercel-Hosted Web Serial Desktop Host**.

Target Hardware: **7Semi ESP32-S3-Dev-BoardC-1U-N8R8**
- **Silicon**: Espressif ESP32-S3 (Xtensa 32-bit dual-core LX7 @ 240 MHz)
- **Module**: ESP32-S3-WROOM-1 MCN8R8
- **Flash**: 8 MB Quad/Octal SPI Flash (DIO, 80 MHz)
- **PSRAM**: 8 MB External PSRAM
- **Native USB**: Hardware USB Serial/JTAG Controller connected via onboard USB-C port (`/dev/cu.usbmodem101` on macOS)
- **Display Hardware**: **None**. The Mac browser serves as the graphical display and host interface over Web Serial.

---

## System Architecture

```text
ESP32-S3 (7Semi N8R8)
         ↓
  SliverOS Runtime
  (Cooperative Scheduler, Memory, VFS, Apps)
         ↓
  SLVR/1 Host Protocol
  (CRC-16-CCITT framed duplex stream)
         ↓
  Native USB Serial/JTAG
         ↓
  Web Serial API
  (Chromium / Google Chrome on macOS)
         ↓
  SliverOS Web Host
         ↓
     Mac Display
```

### ESP32-S3 Responsibilities

- SliverOS kernel and cooperative application scheduler.
- Task control blocks and execution-budget auditing.
- 128 KiB internal SRAM arena and fixed memory pools.
- Block-based OSFS/VFS storage.
- Kernel event bus and fault management.
- Bluetooth LE HID keyboard emulation and macro execution.
- Wi-Fi passive metadata diagnostics.
- Authorized network diagnostics with bounded ICMP/TCP checks.
- Retro game logic and state generation.
- SLVR/1 protocol service and telemetry.
- Watchdog/recovery handling.

### Mac Browser Responsibilities

- Desktop-style graphical interface.
- Application windows and launcher.
- Developer terminal.
- Real-time device monitor.
- Logs and diagnostics.
- Firmware installation.
- Web Serial device connection and permission flow.
- Retro-game Canvas rendering from compact ESP32 game state.
- Simulated-device mode for off-hardware development.

The browser is the host interface; it does not replace SliverOS execution on the ESP32-S3.

---

## The Four Applications

1. **BLE-HID Macro** — BLE keyboard emulation and VFS-backed macro playback.
2. **Wi-Fi Diagnostics** — passive Wi-Fi metadata diagnostics. No PMKID or credential harvesting.
3. **Network Diagnostics** — authorized local-network reachability and bounded service diagnostics.
4. **Retro Games** — game logic on the ESP32-S3 with compact state/input exchange to the browser.

---

## Mac Web Host

### Connection flow

```text
Connect 7Semi ESP32-S3 by USB
            ↓
Open SliverOS Web Host
            ↓
Click "Connect Device"
            ↓
Browser Web Serial permission dialog
            ↓
Select ESP32-S3
            ↓
SLVR/1 handshake
            ↓
SliverOS desktop appears
```

The target development browser is Chrome/Chromium with Web Serial support.

There is **no physical TFT/OLED display requirement** and no framebuffer-to-SPI display path in the intended host architecture.

---

## Verification & Testing

### Host-Native C Test Suite

```bash
make -C tests clean
make -C tests
```

### Web Host

```bash
cd flasher
npm test
npm run build
```

### ESP32-S3

Build and verify the bootloader, partition table, application image, map file, memory placement, Flash geometry, PSRAM configuration, and OTA sizing before treating a build as release-ready.

Do not mark hardware-only tests as passed without physical evidence.

---

## Physical Hardware Test / Experiment Evidence

The supplied photographs document the actual **7Semi ESP32-S3 development board** used as the SliverOS development target and the USB-connected bench setup with the Mac.

**Evidence record:** [`docs/hardware/BOARD_PHOTO_TEST_20260925.md`](./docs/hardware/BOARD_PHOTO_TEST_20260925.md)

The photographs are physical setup evidence only. They do **not** by themselves prove firmware, protocol, scheduler, application, or release-readiness tests.

---

## Repository Structure

```text
SliverOS/
├── os/
│   ├── main/
│   ├── kernel/
│   ├── hal/
│   ├── vfs/
│   ├── protocol/
│   └── apps/
│       ├── ble_hid/
│       ├── wifi_diagnostics/
│       ├── network_diagnostics/
│       └── retro_games/
├── flasher/                    # Web Host + installer
├── tests/
├── tools/
├── scripts/
└── docs/
    ├── ARCHITECTURE.md
    ├── HOST_PROTOCOL.md
    ├── WEB_HOST.md
    ├── HARDWARE_VALIDATION.md
    └── hardware/
        └── BOARD_PHOTO_TEST_20260925.md
```

---

## Safety & Scope

SliverOS is intended for embedded development, authorized diagnostics, defensive security learning, and controlled lab environments.

Network functionality is deliberately bounded and excludes credential harvesting, PMKID capture, password cracking, deauthentication, exploitation automation, and unrestricted attack functionality.

---

## Project Status

| Area | Status |
|---|---|
| 7Semi ESP32-S3 target | 🟢 Defined |
| Cooperative executive | 🟢 Implemented / hardened in stages |
| Mac Web Host architecture | 🟢 Defined |
| Web Serial integration | 🟡 Integration / hardware validation required |
| Four-application model | 🟢 Defined |
| Network diagnostics | 🟡 Hardware validation required |
| Web Flasher | 🟡 Online; physical flashing validation required |
| Physical hardware photo evidence | 🟢 Recorded |
| Full physical hardware validation | 🔴 Pending |
| Release readiness | 🔴 Pending mandatory CI and hardware gates |

## Documentation

- [`docs/ARCHITECTURE_ENFORCEMENT.md`](./docs/ARCHITECTURE_ENFORCEMENT.md)
- [`docs/HARDWARE_VALIDATION.md`](./docs/HARDWARE_VALIDATION.md)
- [`docs/hardware/BOARD_PHOTO_TEST_20260925.md`](./docs/hardware/BOARD_PHOTO_TEST_20260925.md)
- [`VERCEL.md`](./VERCEL.md)
