# 8 MB Flash Partition Table & Space Verification

## Flash Geometry & Sizing Calculation

Physical Flash: 8 MB SPI NOR Flash = `8,388,608` bytes (`0x800000`).
Sector Size: 4,096 bytes (`0x1000`).
Application Alignment Requirement: 64 KB (`0x10000`).

---

## Verified Partition Map (`partitions.csv`)

```text
Name       Type   SubType  Offset    Size (Bytes)   Size (Human)  End Offset  Alignment
──────────────────────────────────────────────────────────────────────────────────────────
bootloader boot   -        0x000000  0x008000       32 KB         0x008000    Sector (4KB)
partitions part   -        0x008000  0x001000        4 KB         0x009000    Sector (4KB)
nvs        data   nvs      0x009000  0x006000       24 KB         0x00F000    Sector (4KB)
otadata    data   ota      0x00F000  0x002000        8 KB         0x011000    Sector (4KB)
phy_init   data   phy      0x011000  0x001000        4 KB         0x012000    Sector (4KB)
(Reserved) -      -        0x012000  0x00E000       56 KB         0x020000    Padding
ota_0      app    ota_0    0x020000  0x280000      2.5 MB         0x2A0000    64KB Aligned
ota_1      app    ota_1    0x2A0000  0x280000      2.5 MB         0x520000    64KB Aligned
osfs       data   spiffs   0x520000  0x2A0000      2.625 MB      0x7C0000    Sector (4KB)
(Margin)   -      -        0x7C0000  0x040000      256 KB         0x800000    End of Flash
──────────────────────────────────────────────────────────────────────────────────────────
Total Allocated: 8,126,464 bytes (7.75 MB) < 8,388,608 bytes (8.0 MB)
Unallocated Safety Margin: 262,144 bytes (256 KB)
```

---

## Verification Assertions

1. **Both OTA Slots Fit**: `ota_0` and `ota_1` both receive a generous 2.5 MB allocation, accommodating standard ESP-IDF applications with Wi-Fi, BLE, and graphics overhead.
2. **Dedicated OSFS Partition**: `osfs` receives 2.625 MB (672 sectors of 4KB), accommodating `macro.txt`, `wifi_audit.log`, and persistent game high scores with power-loss commit markers.
3. **No Overlapping Partitions**: All ranges are contiguous and non-overlapping.
