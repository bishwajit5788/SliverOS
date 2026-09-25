# Building SliverOS

This document details the build processes for the ESP32-S3 firmware, the host-native unit test suites, and the Vercel-hosted Web Host desktop environment.

---

## 1. Target Hardware & Specifications

- **Target Board**: **7Semi ESP32-S3-Dev-BoardC-1U-N8R8**
- **Module**: ESP32-S3-WROOM-1 MCN8R8
- **Target Chip**: `esp32s3`
- **Flash Memory**: 8 MB Quad/Octal SPI Flash (DIO, 80 MHz)
- **External PSRAM**: 8 MB Octal SPI PSRAM (`CONFIG_SPIRAM_MODE_OCT=y`)
- **Native USB**: Hardware USB Serial/JTAG Controller connected via onboard USB-C port
- **Physical Display**: **None**. The Mac browser serves as the graphical display over Web Serial.

---

## 2. Building Firmware with ESP-IDF

### Prerequisites
- ESP-IDF v5.2 installed and sourced in your shell:
  ```bash
  . $HOME/esp/esp-idf/export.sh
  ```
- Python 3.8+ with standard ESP-IDF dependencies.

### Target Selection & Build
```bash
# Ensure target silicon is set to ESP32-S3
idf.py set-target esp32s3

# Compile the bootloader, partition table, and SliverOS executive
idf.py build
```

### Build Artifacts & Memory Offsets
| Artifact | Flash Offset | Description |
|---|---|---|
| `build/bootloader/bootloader.bin` | `0x0000` | ESP32-S3 ROM-compatible first-stage bootloader |
| `build/partition_table/partition-table.bin` | `0x8000` | Custom partition table (8 MB Flash layout) |
| `build/microkernel-esp32.bin` | `0x20000` | SliverOS application binary (fits within 2.5MB OTA slot) |

> [!IMPORTANT]
> The SliverOS application starts at flash offset **`0x20000`** (the `ota_0` partition), **NOT** `0x10000`.

---

## 3. Running Host Unit Tests (Host-Native C)

The core SliverOS executive, cooperative scheduler, memory manager, fixed pools, event bus, VFS block storage with power-loss recovery, application lifecycles, and SLVR/1 protocol engine can be compiled and executed natively on macOS (Apple Silicon/Intel) or Linux without requiring ESP-IDF or hardware:

```bash
# Clean and compile all 12 test suites with Clang/GCC
make -C tests clean
make -C tests

# Or run the compiled test runner directly:
./tests/test_runner
```

### Verified Test Suites (100% Pass Rate):
1. Memory Manager Static Arena Unit Tests
2. Fixed-Size Memory Pool (16B, 64B, 256B) Tests
3. Kernel Event Bus Decoupled Ring Buffer Tests
4. Cooperative Scheduler Priority & Round-Robin Tests
5. State Machine Transition Tests
6. Macro Parser Tokenization Tests
7. VFS Block Storage Allocation & Read/Write Tests
8. VFS Power-Loss Safe Atomic Commit & Recovery Tests
9. Wi-Fi Frame Metadata Classification Tests
10. Network Diagnostics Bounded State Machine Tests
11. Space Micro-Lander Physics & Collision Tests
12. SLVR/1 Binary Protocol Framing, Parsing & Fuzz Tests (200,000 iterations)

---

## 4. Building the SliverOS Web Host

The Web Host frontend is built with Node.js and bundled with Vite:

```bash
cd flasher

# Install dependencies
npm install

# Run Web Host unit tests (manifest, target checks, SLVR/1 frame & stream parser)
npm test

# Build production bundle for Vercel deployment
npm run build

# Start local development server
npm run dev
```

Output directory: `flasher/dist/` (contains standalone `index.html` and bundled assets ready for Vercel hosting).
