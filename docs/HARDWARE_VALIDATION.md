# SliverOS ESP32-S3 Hardware Validation

Target: ESP32-S3-DevKitC-1, 8 MB Flash, 8 MB PSRAM, native USB.

Physical validation cannot be claimed by CI. Each item must be recorded as `PASS`, `FAIL`, or `NOT RUN`, with firmware revision and test date.

| # | Validation | Result |
|---:|---|---|
| 1 | Cold boot reaches graphical launcher | NOT RUN |
| 2 | Warm reset reaches launcher without recovery loop | NOT RUN |
| 3 | Recovery path boots after interrupted application state | NOT RUN |
| 4 | Internal SRAM contains kernel control structures | NOT RUN |
| 5 | 128 KiB arena is physically internal DRAM | NOT RUN |
| 6 | Fixed memory pools are physically internal DRAM | NOT RUN |
| 7 | Allocator detects invalid free/double free | NOT RUN |
| 8 | Scheduler 1 ms timebase continues with runnable tasks | NOT RUN |
| 9 | Scheduler execution budget/overrun telemetry works | NOT RUN |
| 10 | TWDT resets a deliberately wedged executive | NOT RUN |
| 11 | Event queue overflow follows DROP_NEWEST policy | NOT RUN |
| 12 | GPIO/driver callback transport remains bounded | NOT RUN |
| 13 | VFS survives interrupted record write | NOT RUN |
| 14 | VFS selects newest valid record after recovery | NOT RUN |
| 15 | VFS log rotation avoids single-sector hot spot | NOT RUN |
| 16 | BLE HID advertises as a keyboard | NOT RUN |
| 17 | BLE HID accepts a host connection | NOT RUN |
| 18 | BLE HID emits a real key press/release report | NOT RUN |
| 19 | Passive Wi-Fi metadata capture records only allowed metadata | NOT RUN |
| 20 | Network diagnostics performs real asynchronous ICMP | NOT RUN |
| 21 | TCP 22/80/443 probes remain bounded and non-blocking | NOT RUN |
| 22 | Display renders launcher and application frames | NOT RUN |
| 23 | All four applications complete lifecycle start/stop/reset | NOT RUN |
| 24 | Native USB/Web Serial flasher programs firmware | NOT RUN |
| 25 | Firmware SHA-256 artifact verification matches manifest | NOT RUN |

## Evidence requirements

For every physical test, record:

- board identifier and hardware revision;
- firmware git revision;
- date/time;
- test operator;
- observed serial log or measurement;
- result and failure details when applicable.

Do not convert `NOT RUN` to `PASS` from host CI output. Hardware-only checks require physical evidence.
