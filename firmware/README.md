# MicroKernel OS Firmware Artifacts

This directory contains release manifests and pre-packaged binary distributions of the MicroKernel OS for supported ESP32 architectures:

- **ESP32** (Xtensa dual-core, 4MB Flash)
- **ESP32-C3** (RISC-V single-core, 4MB Flash)
- **ESP32-S3** (Xtensa dual-core with vector extensions, 8MB Flash)

## Manifest Structure

Release metadata is maintained in `manifests/releases.json`. Each entry defines:
- **Target Chip Family**: `esp32`, `esp32c3`, `esp32s3`
- **Required Flash Capacity**: Minimum board flash size
- **Flash Images**: Structured list of binary components containing:
  - `name`: Human-readable identifier (e.g., Bootloader, Partition Table, Application)
  - `offset`: Absolute flash memory offset (e.g., `0x1000`, `0x8000`, `0x10000`)
  - `filename`: Image asset filename located in the distribution directory
  - `size`: Expected size in bytes
  - `sha256`: Cryptographic hash for pre-flash integrity verification

## Packaging Workflow

To package new firmware from ESP-IDF build output into the Web Flasher distribution directory:

```bash
python3 scripts/package_firmware.py --build-dir build/ --target esp32 --version 1.0.0
```
