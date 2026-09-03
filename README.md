# MicroKernel OS for ESP32-S3

A **Custom Cooperative Embedded OS Executive running on ESP-IDF**, paired with a **Browser-Based Web Serial Firmware Flasher**.

Targeted primarily for the **ESP32-S3-DevKitC-1** (8MB Octal Flash, 8MB PSRAM, Native USB-Serial/JTAG), with architectural extensibility for future ports to ESP32 and ESP32-C3.

---

## Key Subsystems

1. **Cooperative Round-Robin Scheduler**:
   - Single FreeRTOS executive thread; zero multi-threading jitter or stack bloat.
   - 3-tier arbitration: Priority $\to$ Period Eligibility $\to$ Equal-Priority Round-Robin.
   - Microsecond execution telemetry and budget overrun detection (`last_execution_us`, `worst_execution_us`, `overrun_count`).
   - Hardware Task Watchdog Timer (TWDT) liveness integration.

2. **Dual-Domain Memory Architecture**:
   - **Internal SRAM (Strictly Enforced)**: 128KB static arena allocator (`my_malloc`), 5 fixed-size pool classes (16B–256B), TCBs, fault logs, and event queue. Zero libc heap calls.
   - **External PSRAM (8MB)**: Isolated for display framebuffers and game assets.

3. **Kernel Event Bus**:
   - Decouples hardware interrupts/driver callbacks from cooperative tasks via a static 64-slot bounded ring buffer.

4. **Power-Loss Safe VFS (`osfs`)**:
   - Structured records over 4KB NOR sectors with 4-byte commit markers (`0x55AA55AA`) and CRC32 verification. Discards incomplete writes safely upon mount.

5. **Graphical OS Runtime & Application Launcher (`os/ui/`)**:
   - Replaces the traditional text CLI with a visual application carousel and developer diagnostics telemetry screen (kernel state, SRAM vs. PSRAM stats, task runtimes, display FPS).

6. **The Four Isolated Applications**:
   - **`MK_APP_BLE_HID`**: USB HID keyboard emulation via streaming VFS macro parser.
   - **`MK_APP_WIFI_DIAGNOSTICS`**: Safe passive promiscuous frame metadata auditing (RSSI, channel, frame type/subtype). Strictly no PMKID or credential collection.
   - **`MK_APP_NETWORK_DIAGNOSTICS`**: Non-blocking cooperative state machine auditing ICMP reachability and ports 22, 80, 443 against authorized local targets.
   - **`MK_APP_RETRO_GAMES`**: Space Micro-Lander game engine running at lowest scheduler priority with software-debounced inputs and dirty-region graphics rendering.

7. **Browser Web Serial Firmware Flasher**:
   - Direct-from-browser flashing via Web Serial API (`navigator.serial`) implementing the official ESP32 ROM bootloader protocol.
   - ESP32-S3 primary target detection, SHA-256 pre-verification, test-fixture security guards, real transfer metrics (KB/s, ETA), and on-chip hardware MD5 verification.

---

## Directory Structure

```text
microkernel-esp32/
├── CMakeLists.txt                # ESP-IDF Root CMake build file
├── sdkconfig.defaults            # ESP32-S3 8MB Flash & PSRAM configuration
├── partitions.csv                # 8MB Flash layout with A/B OTA & 2.6MB osfs
├── os/
│   ├── main/main.c               # Boot orchestrator & diagnostic banner
│   ├── kernel/                   # Executive, scheduler, arena, pools, event bus, faults
│   ├── hal/                      # Hardware Abstraction Layer (timer, GPIO, SPI, Wi-Fi, BLE)
│   ├── vfs/                      # Flash storage layer, sector wear-leveling, commit markers
│   ├── ui/                       # Display manager, graphical launcher, developer screen
│   └── apps/
│       ├── ble_hid/              # App 0: BLE Keyboard & Macro Parser
│       ├── wifi_diagnostics/     # App 1: Passive 802.11 Frame Auditor
│       ├── network_diagnostics/  # App 2: Non-blocking Port/Ping State Machine
│       └── retro_games/          # App 3: Space Micro-Lander Game Engine
├── flasher/                      # Vite + Web Serial firmware flashing dashboard
├── tests/                        # Host-native C11 unit test suite (8 test suites)
├── scripts/package_firmware.py   # Firmware release packaging utility
├── docs/                         # Comprehensive engineering documentation
└── .github/workflows/            # CI workflows for host tests, flasher, and IDF build
```

---

## Quick Start & Verification

### 1. Run Host-Native C Unit Tests (macOS / Linux)
```bash
make -C tests
```
*Executes all 8 test suites: Memory Arena, Fixed Pools, Event Bus, Scheduler Round-Robin, State Machine, Macro Parser, VFS Block, and Power-Loss Recovery.*

### 2. Run Web Flasher Unit Tests & Build Production Bundle
```bash
cd flasher
npm test
npm run build
```

### 3. Launch Local Web Flasher Server
```bash
cd flasher
npm run dev
```
Open `http://localhost:3000` in Google Chrome or Microsoft Edge, plug in your ESP32-S3 board, and click **CONNECT DEVICE**.
