# SliverOS Architecture Enforcement

**Revision:** REV-20260904-S3-FROZEN-HARDENED  
**Target:** ESP32-S3-DevKitC-1, 8 MB Flash, 8 MB PSRAM, Native USB

## 1. Cooperative execution and FreeRTOS relationship

ESP-IDF/FreeRTOS remains the underlying vendor runtime and provides the platform executive thread, drivers, interrupts and hardware services. SliverOS does **not** create one FreeRTOS task per application and does not delegate application scheduling to FreeRTOS.

`mk_kernel_run()` owns the application dispatch loop. The SliverOS scheduler alone selects application work by priority, period eligibility and equal-priority round-robin. Application callbacks are non-preemptive and must return voluntarily.

### Global no-blocking rule

The cooperative application path MUST NOT call blocking APIs. This includes `vTaskDelay`, `xQueueReceive(..., timeout)`, blocking socket operations, filesystem waits, semaphore waits, or synchronous peripheral transfers with unbounded latency.

A HAL may use an underlying blocking primitive only behind an explicitly asynchronous/cooperative state machine, and the application callback must return after bounded work. `taskYIELD()` is allowed only as an underlying executive yield; it is not an application scheduling primitive.

The scheduler timebase is monotonic (`esp_timer_get_time()` on target) and is independent of whether an application is runnable. Scheduler ticks therefore cannot stall merely because a task continuously remains READY.

## 2. Event bus overflow policy

The event bus is fixed-size and bounded. Producers never block.

- Capacity: `MK_EVENT_QUEUE_SIZE` (64).
- Full queue policy: **DROP_NEWEST**.
- Existing queued events are preserved.
- `queue_overflows` increments for every rejected event.
- The producer receives `MK_STATUS_QUEUE_FULL`.
- Overflow telemetry is recorded outside the critical section.
- ISR/callback posting performs only bounded copying and index updates.

Future event classes may define coalescing explicitly, but no implicit overwrite policy is permitted.

## 3. Watchdog and failure recovery

The scheduler services the ESP-IDF Task Watchdog only from the cooperative executive loop. A healthy system therefore demonstrates forward progress through repeated scheduler iterations.

Failure policy:

| Failure | Action |
|---|---|
| Allocation exhaustion | Return `NULL`; increment failure accounting; caller recovers or yields. |
| Memory corruption/double free | Record fault; corruption is fatal, double-free is rejected. |
| Event overflow | Drop newest and continue. |
| Scheduler callback >25 ms | Record overrun and continue; repeated overruns are a release blocker. |
| Scheduler/executive hang | TWDT reset; do not attempt unsafe software preemption. |
| VFS mount failure | Enter degraded/headless persistence mode; do not corrupt existing storage. |
| VFS record CRC/commit failure | Ignore incomplete record and recover from the latest valid record. |
| Display I/O failure | Enter headless mode and retain serial diagnostics. |
| BLE/network app fault | Transition application to ERROR, release resources, then reinitialize according to its lifecycle policy. |

A controlled reset must preserve the latest fault information when persistence is available. Hardware reset behavior is validated on physical hardware and is not claimed from host tests.

## 4. VFS write, rotation and recovery policy

The `osfs` partition is a bounded append-oriented record store over ESP-IDF partition APIs. It is not represented as a general-purpose filesystem implementation.

Every persistent record uses:

1. sequence number and length;
2. payload;
3. CRC32;
4. trailing commit marker `0x55AA55AA` written last.

Mount/recovery scans records and accepts only complete, CRC-valid records. Interrupted writes are ignored.

### Wear policy

- Applications may not write flash in high-frequency loops.
- Audit summaries are rate-limited to at least 10 seconds between writes unless a user action explicitly requests persistence.
- A log is rotated before its configured quota is exceeded.
- Rotation must write the replacement record set to clean sectors before reclaiming stale sectors.
- Metadata updates must not repeatedly erase one fixed superblock sector for every application write.
- Recovery must select the newest valid sequence, not simply the newest physical sector.

The current fixed-sector VFS implementation must not claim complete wear leveling until these policies are exercised by tests.

## 5. OTA sizing verification

The partition table allocates:

- `ota_0`: `0x280000` = 2,621,440 bytes.
- `ota_1`: `0x280000` = 2,621,440 bytes.
- `osfs`: `0x2A0000` = 2,752,512 bytes.

CI MUST build the firmware with the checked-in partition table and fail if the generated application image exceeds its OTA slot. CI MUST also print the generated partition table and application binary size so slot headroom is auditable.

No statement that OTA sizing is verified is valid until a real ESP-IDF build has passed this check.

## 6. Web flasher trust model

The browser flasher uses SHA-256 to verify that a selected firmware artifact matches the release manifest. SHA-256 is an **integrity** mechanism when the manifest is trusted; it is not authenticity by itself.

Any ROM/bootloader MD5 or checksum used during transport/programming is treated only as a transfer/programming integrity check. It MUST NOT be described as release authenticity.

Future authenticity is provided by a trusted signature chain such as ESP Secure Boot v2. Production release documentation must preserve this distinction.

## 7. Hardware validation matrix

The release validation matrix contains exactly **25** checks. The count and table must remain synchronized. Checks are grouped as:

1. boot and recovery;
2. memory placement and allocator behavior;
3. scheduler timing and watchdog recovery;
4. event bus and ISR/callback transport;
5. VFS power-loss recovery and wear policy;
6. BLE-HID operation;
7. passive Wi-Fi metadata capture;
8. authorized network diagnostics;
9. display/UI and all four application lifecycles;
10. Web Serial flashing and artifact verification.

Host CI proves only software-level checks. Physical board results require an ESP32-S3 board and must be recorded separately as `PASS`, `FAIL`, or `NOT RUN` with firmware revision and test date.

## 8. Implementation status rule

This document is an enforcement specification, not an approval request. Implementation changes are authorized on the hardening branch. Any item marked implemented must be backed by source code and, where applicable, CI evidence. Hardware-only claims remain `NOT RUN` until physically demonstrated.

Architecture may change only when a build failure, hardware constraint, or test demonstrates that the frozen design is technically invalid. Such a change must be documented before dependent implementation proceeds.
