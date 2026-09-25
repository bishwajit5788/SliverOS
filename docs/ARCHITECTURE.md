# SliverOS: Architectural Specification

## 1. System Identity & Core Philosophy

**SliverOS is a custom cooperative embedded OS/runtime executive for ESP32-S3 built on ESP-IDF.**

SliverOS is neither a bare-metal RTOS nor a Linux-style MMU microkernel. Instead, it provides a deterministic, cooperative application executive that runs on top of the underlying ESP-IDF runtime and FreeRTOS context. ESP-IDF provides low-level silicon drivers (Wi-Fi, Bluetooth/NimBLE, Native USB Serial/JTAG, TWDT, and flash hardware), while SliverOS manages cooperative task arbitration, memory arenas, fixed-size pools, block-based virtual storage, kernel event routing, and the host communication protocol.

### Core Architectural Shift: The Mac Browser as Graphical Display

**There is NO physical SPI TFT/OLED/display hardware in the SliverOS system.**

All concepts of physical display controllers, SPI DMA pixel streaming, local dirty box trackers, and local framebuffers have been eliminated. Instead, the Mac browser serves as the graphical display, window manager, and desktop environment via the Web Serial API over Native USB Serial/JTAG.

```text
┌─────────────────────────────────────────────────────────────┐
│                      ESP32-S3 Hardware                      │
│            7Semi ESP32-S3-Dev-BoardC-1U-N8R8                │
│    Xtensa LX7 @ 240MHz │ 512KB SRAM │ 8MB Octal PSRAM       │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                  SliverOS Firmware Runtime                  │
│  Cooperative Scheduler │ Memory Pools │ Event Bus │ VFS     │
│  BLE HID │ Wi-Fi Diag │ Network Diag │ Retro Game Logic     │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│           SLVR/1 Binary Host Protocol (USB Serial)          │
│       Framing │ Sequence Tracking │ CRC-16-CCITT Guard      │
└──────────────────────────────┬──────────────────────────────┘
                               │ Native USB (/dev/cu.usbmodem101)
┌──────────────────────────────▼──────────────────────────────┐
│                       Web Serial API                        │
│                (Chromium Browser on macOS)                  │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                 SliverOS Web Host Desktop                   │
│   Window Manager │ Vector Canvas │ Terminal │ Monitor       │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Target Hardware: 7Semi ESP32-S3-Dev-BoardC-1U-N8R8

- **Silicon**: Espressif ESP32-S3 (Xtensa 32-bit dual-core LX7 @ 240 MHz)
- **Module**: ESP32-S3-WROOM-1 MCN8R8
- **Flash**: 8 MB SPI Flash (DIO mode, 80 MHz)
- **PSRAM**: 8 MB Octal SPI PSRAM (`CONFIG_SPIRAM_MODE_OCT=y`)
- **Native USB**: Hardware USB Serial/JTAG Controller connected directly to USB-C port
- **Physical USB Port (macOS)**: Discovered dynamically via Web Serial (known port: `/dev/cu.usbmodem101`)
- **Display**: None on-board; Mac browser window serves as the display interface
- **Supported Target**: `esp32s3` strictly enforced; legacy ESP32 classic and ESP32-C3 are rejected by the flasher and build system.

---

## 3. Subsystem Breakdown & Responsibilities

### ESP32-S3 Firmware Responsibilities:
1. **Cooperative Application Scheduler**: Single FreeRTOS executive task scheduling all registered SliverOS tasks non-preemptively based on priorities, periods, and deadlines.
2. **Memory Management**: Custom 128 KiB internal SRAM static arena and fixed memory pools (16B, 64B, 256B) for deterministic $O(1)$ allocation without heap fragmentation.
3. **Power-Loss Safe VFS**: Block/record storage subsystem with atomic commit headers, checksum validation, and wear leveling.
4. **Decoupled Kernel Event Bus**: Bounded ring buffer for asynchronous inter-task signaling and event subscription.
5. **Core Application Engines**:
   - **BLE-HID Macro**: Macro syntax execution and Bluetooth LE keyboard report dispatch.
   - **Wi-Fi Diagnostics**: Passive metadata frame classification (802.11 beacons, probe requests, data traffic) without credential or PMKID harvesting.
   - **Network Diagnostics**: Bounded ICMP reachability checks and TCP port probes (22, 80, 443) with actual network telemetry.
   - **Retro Games**: Embedded Space Micro-Lander physics, gravity, thruster simulation, collision detection, and score tracking.
6. **SLVR/1 Host Protocol Service**: Non-blocking packet decoder, frame serializer, command dispatcher, and telemetry streaming engine.
7. **Fault & Watchdog Management**: Hardware Task Watchdog Timer (TWDT) servicing, overrun detection, and fault record persistence.

### Browser (SliverOS Web Host) Responsibilities:
1. **Web Serial Management**: Secure user-initiated USB device discovery, connection, handshake, and clean disconnection.
2. **Desktop & Window Management**: Modern glassmorphic desktop environment with floating windows, taskbar, dock, and status indicators.
3. **Application Frontends**:
   - BLE Macro file manager, execution monitor, and trigger controls.
   - Wi-Fi signal visualization, AP channel distribution, and frame counters.
   - Network diagnostic latency display, port status badges, and reachability logs.
   - High-performance 60 FPS HTML5 Canvas vector renderer for Space Micro-Lander.
4. **Developer Terminal**: Interactive command console (`root@sliver:~#`) with controlled command dispatching (`help`, `apps`, `status`, `mem`, `vfs`, `reboot`, etc.).
5. **Real-time Device Monitor**: Telemetry graphs displaying CPU tick rate, active tasks, internal SRAM arena usage, PSRAM allocation, VFS blocks, and USB latency.
6. **Firmware Flasher**: Built-in Web Serial bootloader interface for flashing official ESP32-S3 firmware images with SHA-256 integrity verification.

---

## 4. Cooperative Scheduler Architecture

SliverOS avoids multi-threaded FreeRTOS task overhead and lock contention by running a single cooperative scheduler task.

- **Non-blocking Dispatch**: Every registered task executes a short slice and yields immediately. No task is permitted to invoke `vTaskDelay()`, blocking semaphore waits, or unbounded loops.
- **Fixed Task Control Blocks (TCB)**: Statically allocated in Internal SRAM:
  - `TASK_ID_HEARTBEAT` (500 ms period, Priority 3)
  - `TASK_ID_HOST_PROTO` (1 ms period, Priority 0)
  - `TASK_ID_WATCHDOG` (100 ms period, Priority 0)
  - `TASK_ID_EVENT_BUS` (5 ms period, Priority 1)
  - `TASK_ID_VFS_FLUSH` (1000 ms period, Priority 4)
  - `TASK_ID_APP_BLE_HID` (10 ms period, Priority 2)
  - `TASK_ID_APP_WIFI_DIAG` (50 ms period, Priority 2)
  - `TASK_ID_APP_NET_DIAG` (20 ms period, Priority 2)
  - `TASK_ID_APP_RETRO_GAMES` (16 ms period, Priority 1 - 60 Hz physics tick)
- **Overrun Detection**: Real-time timer measurement flags any task execution slice exceeding its deadline and logs a `MK_FAULT_TASK_OVERRUN` event.

---

## 5. Memory Architecture

```text
0x3FC80000 ┌──────────────────────────────────────────────┐
           │        ESP-IDF Runtime & Stack                │
           ├──────────────────────────────────────────────┤
           │   SliverOS 128 KiB Internal SRAM Arena       │
           │   - Kernel TCBs & System State               │
           │   - Fixed-Size Memory Pools (16B, 64B, 256B) │
           │   - Event Bus Queue & Protocol Rx/Tx Buffers │
           │   - DMA-Safe Buffers                         │
0x3FD00000 └──────────────────────────────────────────────┘
           
0x3C000000 ┌──────────────────────────────────────────────┐
           │         8 MB Octal External PSRAM            │
           │   - Application Assets & Large Buffers       │
           │   - Protocol Staging & Telemetry Caches      │
           │   - (ZERO physical display framebuffers)     │
0x3C800000 └──────────────────────────────────────────────┘
```

- **Internal SRAM Isolation**: All critical runtime structures, task control blocks, and communication buffers reside in high-speed internal SRAM.
- **PSRAM Allocation**: External PSRAM is reserved for application data, macro storage, and transient telemetry buffers.

---

## 6. Flash Partition Table (8 MB Layout)

```csv
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x6000,
otadata,    data, ota,     0xf000,   0x2000,
phy_init,   data, phy,     0x11000,  0x1000,
ota_0,      app,  ota_0,   0x20000,  0x280000,
ota_1,      app,  ota_1,   0x2A0000, 0x280000,
osfs,       data, spiffs,  0x520000, 0x2A0000,
```

- **Mathematically Verified**:
  - `ota_0` (Active App): 2.5 MiB (2,621,440 bytes) starting at `0x20000`.
  - `ota_1` (Backup OTA): 2.5 MiB (2,621,440 bytes) starting at `0x2A0000`.
  - `osfs` (VFS Storage): 2.625 MiB (2,752,512 bytes) starting at `0x520000`.
  - End of allocation: `0x7C0000` (7.75 MB), fitting within 8 MB (`0x800000`) with 256 KiB reserve.
