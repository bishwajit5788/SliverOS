# SliverOS Comprehensive Architectural Audit Report

**Date:** 2026-09-25  
**Target Hardware:** 7Semi ESP32-S3-Dev-BoardC-1U-N8R8  
- **SoC:** ESP32-S3 (Xtensa LX7 dual-core @ 240 MHz)  
- **Module:** ESP32-S3-WROOM-1 (MCN8R8)  
- **Memory:** 8 MB Quad/Octal SPI Flash, 8 MB Octal SPI PSRAM  
- **USB:** Dual USB-C (CP2102 USB-UART Bridge + Native USB Serial/JTAG on GPIO 19/20)  
- **Display:** NO physical display hardware attached  
**Repository:** `https://github.com/bishwajit5788/SliverOS`  
**Working Branch:** `feat/sliveros-web-host-reconstruction`

---

## 1. Executive Summary

This audit assesses the state of the codebase, identifies obsolete and conflicting subsystems, details bugs and architectural contradictions, and lays out the precise reconstruction blueprint for **SliverOS**.

SliverOS is defined as:
> **"A custom cooperative embedded OS/runtime executive for ESP32-S3 built on ESP-IDF."**

The previous implementation suffered from a fundamental architectural contradiction: it attempted to drive an unattached physical SPI display (ST7789/OLED) using a local monochrome framebuffer, dirty-box tracker, and SPI DMA driver on GPIO 19/18/23/5/21. Crucially, **GPIO 19 and 20 are the ESP32-S3 Native USB D-/D+ lines**, meaning the display driver conflicted directly with Native USB Serial/JTAG communication. Furthermore, the web application (`flasher/`) was merely an `esptool-js` ROM bootloader installer with zero runtime host interface, no desktop, no terminal, no device telemetry monitor, and no application control windows.

Under the new architecture:
- **No physical display hardware or SPI display pipeline exists.**
- The **Mac browser** acts as the rich graphical display, desktop shell, developer terminal, and telemetry monitor.
- Communication between ESP32-S3 and the Mac browser occurs over **Native USB Serial/JTAG (`/dev/cu.usbmodem101`)** via the **Web Serial API** using a robust, versioned binary protocol (`SLVR/1`).
- The **ESP32-S3** remains the true execution environment: kernel, cooperative scheduler, memory pools, VFS block storage, application state, network/Wi-Fi diagnostics, BLE HID keyboard logic, retro game physics, and fault management.

---

## 2. Current Architecture vs. Reconstructed Target

| Subsystem | Current Repository State | Target Reconstructed Architecture |
|---|---|---|
| **System Branding** | Inconsistent names ("MicroKernel OS" vs "SliverOS") | Unified **SliverOS** executive |
| **Target Hardware** | Generic DevKitC-1 with ungrounded display pins | Grounded **7Semi ESP32-S3-Dev-BoardC-1U-N8R8** (8MB Flash, 8MB PSRAM, Native USB) |
| **Display Pipeline** | Local 128x64 mono framebuffer, dirty box, `hal_spi_write_data` | **Zero physical display.** Mac browser renders GUI via Web Serial host protocol |
| **USB Pin Assignment** | `HAL_SPI_PIN_DC` assigned to GPIO 19 (conflicts with Native USB D-) | GPIO 19/20 preserved exclusively for Native USB Serial/JTAG |
| **Host Interface** | None. Pure UART boot log followed by closed local display loop | Dedicated cooperative **Host Protocol Service** (`SLVR/1`) running as high-priority task |
| **Web Interface** | Standalone firmware flasher (`flasher/`) | **SliverOS Web Host**: Desktop shell, 4 app windows, terminal, device monitor, flasher |
| **Game Architecture** | ESP32 renders pixels into local framebuffer | ESP32 runs game physics/logic; streams compact `MSG_GAME_STATE` to browser Canvas |
| **Network Diagnostics** | Fabricated ICMP ping (`icmp_reachable=true; rtt=12ms`) | Real non-blocking TCP connectivity (22, 80, 443) and live telemetry; zero fake values |
| **Wi-Fi Diagnostics** | Safe passive metadata auditing (management/ctrl/data) | Retained. Real-time telemetry streamed over host protocol to browser visualizer |
| **BLE HID Macro** | Stubbed notification in `hal_ble.c`; NimBLE/Bluedroid config mix | Clean state machine, macro loader from VFS, real status/logs over host protocol |
| **Memory Isolation** | 128 KiB internal SRAM static arena + 8MB PSRAM | Preserved. Internal SRAM for TCBs/queues/buffers; PSRAM for non-critical assets |
| **Storage / VFS** | Sector record layer over `osfs` partition with CRC32 | Retained with accurate terminology (power-loss safe block/record storage layer) |
| **Hardware Flasher** | Multi-chip support (ESP32, ESP32-C3, ESP32-S3) | **Strictly ESP32-S3 only**. Actively rejects ESP32, ESP32-C3, and incompatible chips |

---

## 3. Subsystem-by-Subsystem Audit

### 3.1 Kernel & Cooperative Scheduler
- **State:** Solid foundation with deterministic non-preemptive cooperative scheduling.
- **TCB Allocation:** Fixed array of 8 TCBs in internal SRAM (`MK_MAX_TASKS == 8U`).
- **Scheduling Rules:** Priority-based (0 to 7) with equal-priority round-robin cursor.
- **Runtime Accounting:** Worst-case execution time, last execution time, deadline misses, and budget overrun detection (`MK_TASK_EXEC_BUDGET_US = 25000U`).
- **Defects / Gaps:**
  - `TASK_ID_UI_RUNTIME` currently spends cycles formatting text onto a local monochrome display buffer and driving SPI.
  - Must replace `ui_runtime` task with a high-priority `host_protocol_task` that services the USB Serial/JTAG interface, drains incoming frames, and dispatches commands.

### 3.2 Memory Architecture
- **Internal SRAM:** 128 KiB static arena allocator (`my_malloc`/`my_free`) with bidirectional coalescing, plus fixed-size memory pools (16B, 32B, 64B, 128B, 256B).
- **External PSRAM:** Configured in `sdkconfig.defaults` (`CONFIG_SPIRAM=y`, `CONFIG_SPIRAM_USE_MEMMAP=y`, `CONFIG_SPIRAM_MEMTEST=y`).
- **Defects:**
  - Old documentation claims PSRAM is used for "ST7789 display framebuffers". Since no physical display exists, PSRAM is re-purposed for protocol staging buffers, history logs, and non-critical assets.

### 3.3 Physical Display & SPI Drivers (Obsolete Subsystems)
- **Files to eliminate / refactor:**
  - `os/ui/display_manager.h`, `os/ui/display_manager.c`
  - `os/ui/launcher.h`, `os/ui/launcher.c`
  - `os/ui/ui_runtime.h`, `os/ui/ui_runtime.c`
  - `os/apps/retro_games/renderer.h`, `os/apps/retro_games/renderer.c`
  - `os/hal/hal_spi.h`, `os/hal/hal_spi.c`
- **Findings:**
  - `hal_spi.c` configures SPI pins: MOSI 23, SCLK 18, CS 5, DC 19, RST 21.
  - GPIO 19 is Native USB D- on ESP32-S3! Asserting GPIO 19 corrupts the native USB peripheral.
  - No physical SPI display exists on the 7Semi board.
  - Action: Completely remove the physical display driver and SPI display pipeline.

### 3.4 Host Protocol Service (`SLVR/1`)
- **Current State:** Completely missing.
- **Requirement:** A versioned binary framing protocol over USB Serial/JTAG:
  - Header: `MAGIC` (4 bytes: `SLVR`), `VERSION` (1 byte: `0x01`), `TYPE` (1 byte), `FLAGS` (1 byte), `SEQUENCE` (1 byte), `LENGTH` (2 bytes, big-endian).
  - Body: Up to 512 bytes payload.
  - Footer: `CRC16-CCITT` (2 bytes).
- **Bidirectional Messages:**
  - `MSG_HELLO`, `MSG_DEVICE_INFO`, `MSG_READY`, `MSG_DESKTOP_STATE`, `MSG_APP_STATE`, `MSG_DEVICE_STATUS`, `MSG_MEMORY_STATUS`, `MSG_STORAGE_STATUS`, `MSG_LOG`, `MSG_ERROR`, `MSG_EVENT`, `MSG_PING`, `MSG_PONG`, `MSG_GAME_STATE`, `MSG_TERMINAL_OUTPUT`.
  - Commands: `CMD_CONNECT`, `CMD_GET_INFO`, `CMD_GET_STATUS`, `CMD_LAUNCH_APP`, `CMD_EXIT_APP`, `CMD_APP_INPUT`, `CMD_KEY_EVENT`, `CMD_BUTTON_EVENT`, `CMD_TERMINAL_INPUT`, `CMD_GET_LOGS`, `CMD_RESET`, `CMD_PING`.
- **Integrity:** Constant-time framing validation, bounded length checks, CRC16 verification, malformed frame dropping without crashing.

### 3.5 The Four Core Applications

#### App 1: BLE-HID Macro
- **State:** Reads `macro.txt` from VFS, parses `STRING`, `DELAY`, `ENTER`, `KEY` commands.
- **Defects:**
  - `hal_ble.c` has stubbed `esp_ble_gatts_send_indicate` or report notifications.
  - `sdkconfig.defaults` specifies `CONFIG_BT_NIMBLE_ENABLED=y`, but `hal_ble.c` includes Bluedroid headers (`esp_bt.h`, `esp_gap_ble_api.h`).
- **Reconstruction:** Clean abstraction reporting advertising/connected status and keystroke events to the host protocol, with non-blocking VFS reading.

#### App 2: Wi-Fi Diagnostics
- **State:** Passive 802.11 promiscuous packet sniffer that extracts frame type (mgmt, ctrl, data) and subtype (beacon, probe), computes average RSSI, and hops channels 1–11.
- **Security Check:** Verified clean. Zero PMKID, zero EAPOL handshake interception, zero credential harvesting.
- **Reconstruction:** Expose live statistics via `MSG_APP_STATE` to allow real-time browser visualization of channel activity and frame distribution.

#### App 3: Network Diagnostics
- **Defect Found (Prompt Violation):**
  Lines 111–113 in `os/apps/network_diagnostics/network_diagnostics.c`:
  ```c
  case NET_DIAG_ICMP_PENDING:
      /* Emulate or send ICMP ping */
      s_report.icmp_reachable = true;
      s_report.icmp_rtt_ms = 12U;
      s_report.current_state = NET_DIAG_ICMP_RESULT;
      break;
  ```
  This is hardcoded fake telemetry!
- **Reconstruction:**
  - Eliminate fake ping. If Wi-Fi station is disconnected, report `target_reachable = false`, `icmp_rtt_ms = 0` with state `NET_DIAG_UNAVAILABLE` or `NOT_TESTED`.
  - Perform real non-blocking TCP socket connect checks against ports 22, 80, and 443 with bounded 10ms `select` timeout.

#### App 4: Retro Games (Space Micro-Lander)
- **State:** Realistic 2D vector lander physics (gravity, inertia, fuel burn, landing pad detection, crash threshold).
- **Defect:** Rendered directly into an internal 128x64 display buffer and flushed over SPI DMA.
- **Reconstruction:**
  - Remove all SPI/framebuffer dependencies.
  - Pack lander state (`x`, `y`, `vx`, `vy`, `fuel`, `score`, `status`, `terrain_x`, `terrain_y`, `pad_x`, `pad_w`) into a 24-byte `MSG_GAME_STATE` packet.
  - Transmit packet over USB to the browser at ~20 Hz.
  - Browser renders an authentic retro lunar surface, lander polygon, thrust flame, particle debris, and score HUD on an HTML5 Canvas.
  - Browser sends keyboard input (`UP`, `LEFT`, `RIGHT`, `RESET`) back to ESP32 via `CMD_APP_INPUT`.

### 3.6 Web Interface & Flasher (`flasher/` -> "SliverOS Web Host")
- **State:** Currently only an `esptool-js` Web Serial flasher.
- **Defects:**
  - No desktop interface.
  - Allows selecting ESP32 and ESP32-C3 targets instead of strictly ESP32-S3.
  - No connection workflow for a running SliverOS board.
- **Reconstruction:**
  - Build the unified **SliverOS Web Host**:
    1. **Top Bar:** SliverOS Branding, Connection State, ESP32-S3 Identity, Firmware Version, USB link status.
    2. **Sidebar:** Home / Desktop, Apps, Developer Terminal, Device Monitor, Logs, Storage, Flasher.
    3. **Application Windows:**
       - BLE-HID Macro: Macro file selector, macro status, run/stop trigger.
       - Wi-Fi Diagnostics: Live frame distribution chart, RSSI gauge, channel occupancy.
       - Network Diagnostics: Target host configuration, live TCP reachability (22/80/443), status report.
       - Retro Games: HTML5 Canvas retro game rendering, gamepad/keyboard controls, real physics driven by ESP32.
    4. **Terminal:** Interactive command shell (`help`, `apps`, `status`, `mem`, `tasks`, `vfs`, `logs`, `version`, `reboot`, `clear`).
    5. **Device Monitor:** Real-time CPU tick, task runtime breakdown, internal SRAM vs. PSRAM allocation graphs, fault log viewer.
    6. **Flasher:** Preserved esptool-js engine targeting **strictly ESP32-S3**, verifying SHA-256 integrity, rejecting incompatible chips.
    7. **Host Simulator Mode:** Full in-browser simulated device ("SIMULATED DEVICE" badge) for development and testing without hardware.

### 3.7 Partition Table & Memory Map Verification
- `partitions.csv` Layout:
  - `nvs`: `0x9000` (24 KB / 0x6000)
  - `otadata`: `0xF000` (8 KB / 0x2000)
  - `phy_init`: `0x11000` (4 KB / 0x1000)
  - `ota_0`: `0x20000` (2.5 MB / 0x280000)
  - `ota_1`: `0x2A0000` (2.5 MB / 0x280000)
  - `osfs`: `0x520000` (2.625 MB / 0x2A0000)
  - Total end: `0x7C0000` (7.75 MB).
  - Flash Capacity: 8 MB (`0x800000`).
  - **Verdict:** Valid. Fits cleanly within 8 MB with 256 KB safety margin.

---

## 4. Prioritized Reconstruction Plan

1. **Commit 1: Audit existing architecture** (this report).
2. **Commit 2: Remove physical display and obsolete SPI dependencies.**
   - Eliminate `os/ui/display_manager.*`, `os/ui/launcher.*`, `os/ui/ui_runtime.*`.
   - Remove `hal_spi.*` and decouple GPIO 19/20 for Native USB.
   - Refactor `retro_games` to decouple renderer from game physics.
3. **Commit 3: Add versioned host protocol (`SLVR/1`).**
   - Header definitions, message types, CRC16-CCITT routines, encoder and decoder with boundary checks.
4. **Commit 4: Add ESP32 host protocol service.**
   - Non-blocking USB Serial/JTAG communication task integrated into the cooperative scheduler.
   - Command dispatcher (`LAUNCH_APP`, `APP_INPUT`, `TERMINAL_INPUT`, `GET_STATUS`).
5. **Commit 5: Implement Web Serial transport & host protocol in web interface.**
   - TypeScript/JavaScript SLVR/1 protocol framing parser and encoder.
   - Connection lifecycle (detection, Web Serial picker, handshake, disconnect recovery).
6. **Commit 6: Reconstruct web host desktop & application windows.**
   - Desktop view, Top Bar, Sidebar, App Windows (BLE-HID, Wi-Fi, Network, Retro Games Canvas).
   - Real-time Device Monitor and Developer Terminal.
7. **Commit 7: Build host simulator mode.**
   - In-browser simulated device with "SIMULATED DEVICE" badge for offline testing.
8. **Commit 8: Clean up firmware applications & remove fake diagnostics.**
   - Eliminate fake ICMP ping from `network_diagnostics.c`.
   - Update `ble_hid` and `wifi_diagnostics` telemetry.
9. **Commit 9: Update test suites & CI workflows.**
   - Add host protocol unit tests and framing fuzz tests to `tests/`.
   - Add web protocol tests to `flasher/test/`.
   - Update GitHub Actions workflows.
10. **Commit 10: Rewrite documentation & update hardware validation matrix.**
    - Update `ARCHITECTURE.md`, `HOST_PROTOCOL.md`, `WEB_HOST.md`, `HARDWARE_VALIDATION.md`.
