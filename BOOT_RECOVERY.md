# Boot Sequence & Recovery Architecture

## Overview

MicroKernel OS operates on top of the immutable silicon ROM bootloader provided by Espressif. The execution flow is strictly staged to ensure fail-safe recovery even in cases of severe flash corruption or application crashes.

---

## Normal Boot Progression

```text
[Silicon Power-On / EN Reset]
             │
             ▼
   ESP32 ROM Bootloader
   - Checks Strapping Pins (GPIO 0 / DTR/RTS)
   - Reads Flash Offset 0x0000 / 0x1000
             │
             ▼
   ESP-IDF 2nd-Stage Bootloader
   - Reads Partition Table (0x8000)
   - Inspects otadata (0xF000) for Active App Slot (ota_0 / ota_1)
   - Validates Image Header & SHA-256 Digest
             │
             ▼
   Application Image Entry (0x20000)
   - Initializes C Runtime, BSS, Data Segments
   - Mounts PSRAM (8MB Octal)
   - Launches Single FreeRTOS Executive Thread
             │
             ▼
   MicroKernel OS Executive (`app_main()`)
   - `mk_kernel_boot()`: Transitions state RESET -> INIT
   - `mk_memory_init()`: Configures 128KB static arena in Internal SRAM
   - `mk_pool_init()`: Sets up fixed pools (16B, 32B, 64B, 128B, 256B)
   - `mk_event_bus_init()`: Allocates 64-event static ring buffer
   - `hal_init()`: Configures GPIOs, SPI display, Timer
   - `vfs_init()`: Mounts `osfs` partition, validates commit markers
   - `ui_runtime_init()`: Launches Graphical App Launcher
   - `app_register_all()`: Registers 4 applications
   - `mk_kernel_run()`: Enters non-preemptive cooperative round-robin loop
```

---

## Fault & Boot Failure Recovery

```text
[Application Panic / WDT Reset / Unhandled Fault]
                         │
                         ▼
             ESP32 Task Watchdog (TWDT)
                         │
                         ▼
               Automatic System Reset
                         │
                         ▼
        ESP-IDF 2nd-Stage Bootloader Rollback
  - Increments boot failure counter in `otadata`
  - If boot count exceeds limit, rolls back to alternate OTA slot
                         │
                         ▼
           [If Both Slots Corrupted]
                         │
                         ▼
               Hardware Recovery Mode
   - Connect ESP32-S3 via Native USB data cable
   - Open Web Serial Flasher in Chrome/Edge
   - Auto-reset pulls GPIO 0 LOW and pulses EN
   - Silicon ROM Bootloader executes directly from ROM
   - Re-flash pristine partition table and MicroKernel OS
```

---

## The ROM Bootloader Invariant

Because the first-stage ROM bootloader is etched permanently into the silicon die at manufacturing time:
- An application crash or bad flash write **cannot** overwrite or brick the ROM bootloader.
- The Web Serial Flasher is always guaranteed to re-acquire control over the device via the DTR/RTS auto-reset circuit (or holding the physical BOOT button while clicking RST).
