# SliverOS Hardware Validation & Bench Matrix

## 1. Target Hardware Specification

- **Board**: **7Semi ESP32-S3-Dev-BoardC-1U-N8R8**
- **Module**: ESP32-S3-WROOM-1 MCN8R8
- **Processor**: Xtensa 32-bit LX7 Dual-Core @ 240 MHz
- **Flash Memory**: 8 MB Quad/Octal SPI Flash (DIO Mode, 80 MHz)
- **External PSRAM**: 8 MB Octal SPI PSRAM (`CONFIG_SPIRAM_MODE_OCT=y`)
- **USB Interface**: Native USB Serial/JTAG Controller connected to onboard USB-C port
- **Host System**: Apple Silicon / Intel Mac running macOS
- **Known Working Serial Port**: `/dev/cu.usbmodem101`
- **Display Hardware**: None. All graphics, windows, and telemetry are rendered on the Mac browser via Web Serial.

---

## 2. Hardware Validation Policy

> [!IMPORTANT]
> **VALIDATION STATUS DISCLOSURE**
> All items below that have not been physically verified on the connected 7Semi ESP32-S3 hardware are strictly marked as **NOT RUN**. Never convert `NOT RUN` to `PASS` without physical bench telemetry and serial output evidence.
>
> Legend:
> - `NOT RUN`: Test defined and prepared, awaiting physical execution on target bench.
> - `PASS`: Physical hardware test executed, validated by actual log traces.
> - `FAIL`: Hardware or firmware execution failed acceptance criteria.
> - `BLOCKED`: Dependency or external hardware requirement missing.

---

## 3. 25-Point Hardware Bench Test Matrix

| # | Test Item | Target / Subsystem | Status | Verification Criteria & Method |
|:---:|---|---|:---:|---|
| 1 | Cold Boot Stabilization | Hardware / Power | `NOT RUN` | Connect USB-C cable to Mac; verify 3.3V rail stabilizes and status LED illuminates. |
| 2 | Warm Reboot (RST Button) | Silicon Reset | `NOT RUN` | Press onboard RST button; confirm clean reboot without memory retention faults or latch-up. |
| 3 | Native USB Enumeration | USB Controller | `NOT RUN` | macOS `ioreg` / `system_profiler` identifies Espressif USB JTAG/serial debug unit (`/dev/cu.usbmodem101`). |
| 4 | ROM Bootloader Entry | Bootloader | `NOT RUN` | Flasher sends DTR/RTS auto-reset sequence; ESP32-S3 ROM bootloader synchronizes @ 115200 baud. |
| 5 | Target Silicon Identification | Chip Registers | `NOT RUN` | Query silicon register `0x60007000`; confirms `ESP32-S3` revision and rejects incompatible chips. |
| 6 | 8 MB Flash Geometry | SPI Flash | `NOT RUN` | `SPI_ATTACH` reads manufacturer ID and verifies 8 MB Flash capacity. |
| 7 | 8 MB Octal PSRAM Detection | External SPIRAM | `NOT RUN` | Boot log confirms `SPIRAM: Found 8192K Octal PSRAM` initialized and mapped to `0x3C000000`. |
| 8 | Firmware Flashing via Web Serial | Flasher Engine | `NOT RUN` | FlashManager writes bootloader (`0x0`), partition table (`0x8000`), and SliverOS app (`0x20000`). |
| 9 | ROM Hardware MD5 Validation | Flash Integrity | `NOT RUN` | ROM bootloader computes MD5 hash over written sectors and matches host binary digest. |
| 10 | Post-Flash Executive Boot | SliverOS Kernel | `NOT RUN` | Firmware boots from `0x20000`; initializes 128 KiB SRAM arena and enters READY state. |
| 11 | SLVR/1 Protocol Handshake | USB Protocol Service | `NOT RUN` | Web Host sends `SLVR_CMD_CONNECT`; firmware replies with `SLVR_MSG_HELLO` (version `0x01`). |
| 12 | Browser Device Authentication | Web Host Client | `NOT RUN` | Browser queries `SLVR_CMD_GET_INFO`; validates target name `"7Semi ESP32-S3-N8R8"`. |
| 13 | Live Disconnect / Reconnect | Web Serial Transport | `NOT RUN` | Unplugging USB cable returns Web UI cleanly to `DISCONNECTED`; reconnecting re-establishes session without page refresh. |
| 14 | Cooperative Scheduler Arbitration | Scheduler | `NOT RUN` | Sched tick advances deterministically across tasks; priority order maintained without starvation. |
| 15 | Task Overrun Detection | Fault Manager | `NOT RUN` | Intentional task delay triggers `MK_FAULT_TASK_OVERRUN` without FreeRTOS panic. |
| 16 | Hardware TWDT Servicing | Silicon Watchdog | `NOT RUN` | Executive periodically feeds Task Watchdog Timer; stopping scheduler triggers hardware reset after 5.0s. |
| 17 | 128 KiB Static Arena Integrity | Memory Manager | `NOT RUN` | SRAM allocations allocate deterministically from static arena; zero libc heap fragmentation. |
| 18 | Power-Loss Safe VFS Operations | VFS Storage | `NOT RUN` | Write records to `osfs` partition; power-cycle mid-operation; verify atomic rollback on restart. |
| 19 | BLE-HID Macro Execution | BLE Subsystem | `NOT RUN` | Connect ESP32-S3 as Bluetooth LE keyboard to Mac; trigger macro; verify keystrokes arrive in Mac text editor. |
| 20 | Wi-Fi Passive Diagnostics | Wi-Fi Subsystem | `NOT RUN` | Sniffer captures local 802.11 beacons and probe requests; displays RSSI/channel metadata on Web Host. |
| 21 | Safe Network Diagnostics | Network Subsystem | `NOT RUN` | Real non-blocking ICMP reachability check and TCP port 80/443 probes report real RTT to Web Host. |
| 22 | Retro Game Logic & Vector Stream | Retro Games Engine | `NOT RUN` | Space Micro-Lander physics tick @ 60 Hz on ESP32; streams compact `SLVR_MSG_GAME_STATE` to browser Canvas. |
| 23 | Web Host Canvas Render Rate | Web Host UI | `NOT RUN` | Browser renders lunar lander craft and lunar terrain at sustained 60 FPS without frame drops. |
| 24 | Developer Terminal Console | Shell Subsystem | `NOT RUN` | Commands `help`, `apps`, `status`, `mem`, `vfs`, `reboot` execute via `SLVR_CMD_TERMINAL_INPUT`. |
| 25 | Malformed Packet Rejection | Protocol Robustness | `NOT RUN` | Fuzz frames with invalid length, bad magic, or corrupted CRC are silently rejected without firmware crash. |
