# System Failure Classification & Recovery Model

## Overview

MicroKernel OS categorizes all software and hardware runtime exceptions into three distinct containment tiers to ensure maximum system availability and prevent application errors from taking down the core executive.

---

## Failure Classification Matrix

| Failure Event | Subsystem | Severity Classification | Kernel Containment & Recovery Policy |
|---|---|:---:|---|
| **Memory Exhaustion (Arena)** | Memory Manager | `RECOVERABLE` | Return `NULL` to caller; increment `failed_allocations`; caller frees resources or yields. |
| **Arena Block Corruption** | Memory Manager | `FATAL` | Canary check detects corrupted header; logs `MK_FAULT_MEMORY_CORRUPTION`; controlled TWDT system reset. |
| **Memory Double-Free** | Memory Manager | `RECOVERABLE` | Rejection of free operation; logs warning; caller pointer lifecycle quarantined. |
| **Event Bus Overflow** | Event Bus | `RECOVERABLE` | Drop newest event; increment `queue_overflows`; log warning; executive continues uninterrupted. |
| **VFS Mount Failure** | Virtual File System | `DEGRADED` | OS falls back to in-memory pseudo-FS; allows Wi-Fi/Game/BLE to operate without persistent storage. |
| **VFS Record Corruption** | Virtual File System | `RECOVERABLE` | Missing commit marker or bad CRC32; record skipped; subsequent valid records mounted. |
| **BLE Radio Stack Error** | Radio / HAL | `RECOVERABLE` | App transitions to `MK_APP_STATE_ERROR`; restarts advertising after 3-second backoff. |
| **Wi-Fi Frame RX Overflow** | Radio / HAL | `RECOVERABLE` | Drop oldest metadata record; increment drop counter; non-critical packet metrics preserved. |
| **Network Socket Timeout** | Network Diag | `RECOVERABLE` | Non-blocking socket timeout; marks target port closed; state machine advances to next step. |
| **Display I/O Failure** | Graphical UI | `DEGRADED` | UI runtime flags display unavailable; executive transitions to headless mode; dev telemetry on serial. |
| **Scheduler Task Overrun** | Scheduler | `RECOVERABLE` | Task exceeded 25ms budget; increment `overrun_count`; log fault; task yields on next tick. |
| **Runaway Task / Hang** | Scheduler | `FATAL` | Task fails to voluntarily yield within 5.0 seconds; Task Watchdog Timer (TWDT) triggers hardware reboot. |
| **Invalid State Transition** | State Machine | `RECOVERABLE` | State machine rejects transition; logs `MK_FAULT_STATE_INVALID_TRANSITION`; app remains in current state. |

---

## Containment Rules

1. **Subsystem Isolation**: A crash or buffer exhaustion in an application (e.g. Retro Games or Network Diag) CANNOT corrupt the kernel scheduler or memory allocator.
2. **Degraded Mode Execution**: If the SPI display or VFS flash partition fails during boot, MicroKernel OS will still boot into degraded mode rather than halting immediately, allowing diagnostics over the Native USB serial console.
3. **Fault Telemetry Preservation**: Prior to any controlled reset, fault records are committed to the 32-slot static ring buffer. On subsequent boot, previous-fault telemetry is retrieved and displayed on the developer diagnostics screen.
