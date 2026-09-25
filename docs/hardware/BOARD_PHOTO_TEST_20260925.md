# SliverOS Hardware Photo & Connection Evidence — 2026-09-25

## Purpose

This record documents the physical SliverOS development target and the USB-connected bench setup shown in the supplied photographs.

## Test / Experiment

**Experiment:** SliverOS ESP32-S3 hardware identification and Mac USB connection evidence

**Date:** 2026-09-25

**Target board:** 7Semi ESP32-S3 Development Board

**MCU/module visible in photographs:** ESP32-S3-WROOM-1

**Host:** MacBook

**Display hardware:** None. SliverOS graphical interaction is intended to use the Mac browser/host interface over USB.

## Supplied Photographic Evidence

### 01 — Board front

![7Semi ESP32-S3 board front](../../assets/hardware/20260925/01_board_front.jpeg)

The photograph shows the front side of the 7Semi ESP32-S3 development board, including the ESP32-S3-WROOM-1 module, antenna area, labeled GPIO headers, BOOT/RST buttons, status LED area, and USB connectors.

### 02 — Board connected to Mac

![7Semi ESP32-S3 connected to Mac](../../assets/hardware/20260925/02_board_usb_connected.jpeg)

The photograph shows the development board connected by USB while positioned in front of the Mac.

### 03 — ESP32-S3 module detail

![ESP32-S3 module detail](../../assets/hardware/20260925/03_board_detail.jpeg)

Close-up evidence of the ESP32-S3-WROOM-1 module and the 7Semi board layout.

### 04 — Connected board / bench view

![Connected 7Semi ESP32-S3 bench view](../../assets/hardware/20260925/04_board_connected.jpeg)

Additional bench photograph showing the board connected by USB in the Mac development environment.

## Evidence Status

These photographs are **physical evidence of the board and bench setup**, not proof that every item in the hardware validation matrix has passed.

The following remain separate validation claims and must only be marked `PASS` after their corresponding test is actually executed and recorded:

- firmware flashing
- SliverOS boot
- SLVR/1 protocol handshake
- Web Serial host connection
- 8 MB Flash validation
- 8 MB PSRAM validation
- scheduler behavior
- watchdog behavior
- VFS behavior
- BLE-HID execution
- Wi-Fi diagnostics
- network diagnostics
- retro-game protocol/rendering
- disconnect/reconnect
- malformed packet rejection

## Related Validation Matrix

See [`docs/HARDWARE_VALIDATION.md`](../HARDWARE_VALIDATION.md) for the authoritative 25-point hardware test matrix and its `NOT RUN` / `PASS` / `FAIL` / `BLOCKED` rules.
