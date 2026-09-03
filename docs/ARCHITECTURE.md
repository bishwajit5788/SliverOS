# MicroKernel OS: Architectural Specification

## 1. System Identity & Correct Terminology

MicroKernel OS is a **Custom Cooperative Embedded OS Executive running on ESP-IDF**.

Under no circumstances does MicroKernel OS claim to replace the underlying ESP-IDF runtime or silicon ROM bootloader. ESP-IDF provides low-level hardware drivers (Wi-Fi, Bluetooth, SPI, USB, TWDT) and the single FreeRTOS context within which the executive runs.

MicroKernel OS provides:
- Cooperative multi-task scheduling with priority, period, and round-robin arbitration.
- Deterministic 128KB static arena allocator in Internal SRAM with zero libc heap fragmentation.
- Fixed-size memory pools (16B–256B) for constant-time O(1) allocation.
- Decoupled asynchronous Kernel Event Bus with bounded ring buffering.
- Graphical application runtime and launcher replacing traditional text shells.
- Power-loss safe flash virtual file system with explicit commit markers.
- Multi-category fault auditing subsystem.
- Browser-based Web Serial firmware installer directly targeting silicon ROM.

---

## 2. Hardware Target Hierarchy

- **PRIMARY / RELEASE DEVELOPMENT TARGET**: **ESP32-S3-DevKitC-1**
  - Xtensa 32-bit LX7 Dual-Core @ 240 MHz
  - 8 MB Octal SPI Flash
  - 8 MB Octal SPI PSRAM
  - Native USB Serial/JTAG Controller
- **FUTURE PORTING TARGETS**:
  - ESP32 Classic (Xtensa LX6)
  - ESP32-C3 (RISC-V 32-bit RV32IMC)

---

## 3. Subsystem Architecture

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        ESP32-S3-DevKitC-1 Hardware                     │
│  Xtensa LX7 @ 240MHz │ 512KB Internal SRAM │ 8MB PSRAM │ 8MB SPI Flash │
└────────────────────────────────────┬───────────────────────────────────┘
                                     │
┌────────────────────────────────────▼───────────────────────────────────┐
│                      ESP-IDF Runtime Environment                       │
│     ROM Bootloader │ Single FreeRTOS Executive Thread │ Hardware ISRs  │
└────────────────────────────────────┬───────────────────────────────────┘
                                     │
┌────────────────────────────────────▼───────────────────────────────────┐
│                    Hardware Abstraction Layer (HAL)                    │
│   hal_timer (us) │ hal_gpio (debounce) │ hal_spi │ hal_wifi │ hal_ble  │
└────────────────────────────────────┬───────────────────────────────────┘
                                     │
┌────────────────────────────────────▼───────────────────────────────────┐
│                    MicroKernel Executive Core (Internal SRAM)          │
│   ┌─────────────────────┐  ┌────────────────────┐  ┌────────────────┐  │
│   │ 128KB Static Arena  │  │ Fixed Pools 16-256B│  │ Event Bus (64) │  │
│   └─────────────────────┘  └────────────────────┘  └────────────────┘  │
│   ┌─────────────────────┐  ┌────────────────────┐  ┌────────────────┐  │
│   │ Round-Robin Sched   │  │ Fault Manager (32) │  │ Power-Safe VFS │  │
│   └─────────────────────┘  └────────────────────┘  └────────────────┘  │
└────────────────────────────────────┬───────────────────────────────────┘
                                     │
┌────────────────────────────────────▼───────────────────────────────────┐
│                  Graphical OS Runtime & Application Layer               │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │      os/ui: Display Manager, App Launcher, Developer Screen    │   │
│   └────────────────────────────────┬───────────────────────────────┘   │
│                                    │                                   │
│   ┌───────────────┬────────────────┴┬─────────────────┬────────────┐   │
│   │ App 0:        │ App 1:          │ App 2:          │ App 3:     │   │
│   │ BLE-HID Macro │ Wi-Fi Passive   │ Network Diag    │ Retro      │   │
│   │ Keyboard      │ Diagnostics     │ State Machine   │ Games      │   │
│   └───────────────┴─────────────────┴─────────────────┴────────────┘   │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Subsystem Descriptions

### Memory Architecture
- **Internal SRAM Only**: The 128KB static arena (`my_malloc`), 5 memory pool classes, TCB table, fault ring buffer, and event queue remain strictly inside internal zero-wait-state SRAM.
- **External PSRAM**: Reserved exclusively for non-critical bulk buffers (e.g. display framebuffers, game assets).

### The Kernel Event Bus (`event_bus.c`)
Decouples hardware interrupts from cooperative tasks. Callbacks quickly post a 24-byte event struct to a static 64-slot ring buffer. Tasks consume events cooperatively.

### The Cooperative Scheduler (`scheduler.c`)
Executes the highest priority eligible ready task, with equal-priority tasks arbitrated using round-robin cursor rotation. Overruns against the 25ms budget are detected and logged.

### Power-Loss Safe VFS (`vfs_block.c`)
Append records use a trailing 4-byte commit marker (`0x55AA55AA`) written only after the payload and CRC are successfully committed to flash. Incomplete writes caused by power loss are detected and discarded cleanly upon mount.

### Graphical OS Runtime (`os/ui/`)
Replaces the traditional text shell with a visual application launcher and real-time developer diagnostics screen displaying kernel state, memory metrics, task execution times, radio telemetry, and display FPS.

---

## 5. Architecture Freeze Invariants

```text
================================================================================
ARCHITECTURE REVISION: REV-20260904-S3-FROZEN
================================================================================
1. PRIMARY SILICON: ESP32-S3-DevKitC-1 (8MB Flash, 8MB PSRAM, Native USB).
2. SCHEDULER: Custom cooperative scheduler is sole application authority (NO per-app FreeRTOS tasks).
3. APPLICATIONS: Exactly four isolated applications (BLE_HID, WIFI_DIAG, NET_DIAG, RETRO_GAMES).
4. MEMORY SEPARATION: Critical structures in Internal SRAM BSS; PSRAM for framebuffers/assets.
5. STORAGE: Power-loss safe records over osfs with 0x55AA55AA commit markers.
6. INTERRUPTS: Minimal ISRs posting to 64-slot static event bus.
7. FLASHER SECURITY: SHA-256 integrity + test-fixture rejection in production mode.
================================================================================
```
