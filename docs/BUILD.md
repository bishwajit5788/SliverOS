# Building MicroKernel OS

This document details the build processes for both target ESP32 hardware firmware and host unit test environments.

---

## 1. Building Firmware with ESP-IDF

### Prerequisites
- ESP-IDF v4.4, v5.0, v5.1, or v5.2 installed and sourced.
- Python 3.8+ with standard ESP-IDF virtual environment.

### Target Selection & Configuration
Configure target architecture (ESP32, ESP32-C3, or ESP32-S3):
```bash
# Sourcing ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Set chip target
idf.py set-target esp32
# or: idf.py set-target esp32c3
# or: idf.py set-target esp32s3
```

### Compilation
```bash
idf.py build
```

This compiles:
1. `build/bootloader/bootloader.bin` (First-stage bootloader, loaded at offset `0x1000` or `0x0000`)
2. `build/partition_table/partition-table.bin` (Custom partition map from `partitions.csv`, offset `0x8000`)
3. `build/microkernel-esp32.bin` (MicroKernel OS application binary, offset `0x10000`)

---

## 2. Packaging Firmware for the Web Flasher

After a successful ESP-IDF build, run the packaging script:
```bash
python3 scripts/package_firmware.py --build-dir build/ --target esp32 --version 1.0.0
```

This command:
- Computes SHA-256 cryptographic digests of each binary image.
- Copies the images into `flasher/public/firmware/`.
- Updates `firmware/manifests/releases.json` and copies it into the Web Flasher distribution directory.

To generate standalone development binaries without an active ESP-IDF installation:
```bash
python3 scripts/package_firmware.py --generate-sample --target all
```

---

## 3. Running Host Unit Tests

The core executive, scheduler, static arena memory allocator, state machine, macro parser, and VFS block storage can be verified natively on macOS or Linux without needing target hardware or an ESP-IDF cross-compiler:

```bash
make -C tests
```

To clean test artifacts:
```bash
make -C tests clean
```

---

## 4. Building the Web Flasher

```bash
cd flasher
npm install
npm run build
```

This bundles the static HTML, CSS, and ES modules into `flasher/dist/` with zero server dependencies.
