# SPI NOR Flash Wear-Leveling & Write Policy

## Physical Flash Constraints

The ESP32-S3 uses SPI NOR Flash (Winbond or Macronix series):
- **Erase Granularity**: 4,096 bytes (4 KB sector).
- **Write Granularity**: 4 bytes (32-bit word). Programming can only transition bits from `1` to `0`.
- **Endurance Limit**: ~100,000 erase/write cycles per sector. Continuous unbounded writes to a single sector would degrade flash in weeks.

---

## Operating System Write Policies

### 1. Bounded Write Frequency
- **Rule**: Applications are strictly prohibited from writing to the VFS flash partition in high-frequency loops.
- **Logging Limits**:
  - `wifi_diagnostics`: Writes an aggregate audit summary at most once every 10 seconds.
  - `network_diagnostics`: Writes audit results only upon scan sequence completion.
  - Applications writing more frequently than once every 5 seconds without user interaction are rejected during review.

### 2. File Size Caps & Log Rotation
- **Maximum Log File Size**: 128 KB (32 sectors).
- **Rotation Behavior**: When `wifi_audit.log` reaches 128 KB, the VFS rotates the log by copying the newest 16 KB to a clean sector and freeing stale sectors.
- **Quota Allocation**:
  - `macro.txt`: 4 KB maximum (Sector 1)
  - `wifi_audit.log`: 128 KB maximum (Sectors 2–33)
  - User Data & Save States: Remaining 2.4 MB of `osfs` partition

### 3. Power-Loss Safe Write Protocol
- Every record is written sequentially with:
  1. Header with length and sequence number.
  2. CRC32 calculated over payload.
  3. Trailing 4-byte commit marker (`0x55AA55AA`).
- Writes interrupted by sudden power disconnect will lack the commit marker and are skipped during next boot, preventing corrupt data from polluting log files.
