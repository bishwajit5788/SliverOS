# Virtual File System & Flash Storage Architecture

## Storage Physical Realities

MicroKernel OS interacts with raw SPI NOR flash. Unlike magnetic disks or eMMC flash with internal Flash Translation Layers (FTL):
- **Erase Granularity**: 4,096 bytes (4KB sectors). Individual bytes cannot be erased independently.
- **Write Granularity**: 4 bytes (32-bit words). Programming can only flip bits from `1` to `0`. Transitioning a bit from `0` to `1` requires a full 4KB sector erase.
- **Endurance**: NOR flash sectors sustain approximately 100,000 erase cycles before degradation.

---

## Flash Partitioning & Layout

The dedicated 2MB partition is labeled `osfs` in `partitions.csv`:
```text
osfs, data, spiffs, 0x190000, 0x200000,
```

Sector allocation:
- **Sector 0**: Superblock and file inode directory table.
- **Sector 1**: Pre-seeded default macro configuration file (`macro.txt`).
- **Sector 2**: Structured Wi-Fi and network audit log (`wifi_audit.log`).
- **Sectors 3 to 511**: Dynamic data sectors for user files, game high scores, and persistent logs.

---

## Superblock & Inode Management

```c
typedef struct {
    char name[VFS_MAX_FILENAME_LEN];
    uint32_t start_sector;
    uint32_t size_bytes;
    uint32_t flags;
    uint32_t in_use;
} vfs_inode_t;

typedef struct {
    uint32_t magic;         /* 0x4F534653 ("OSFS") */
    uint32_t file_count;
    vfs_inode_t inodes[VFS_MAX_FILES];
    uint32_t crc32;
} vfs_superblock_t;
```

---

## Corruption Detection & Resilience

1. **CRC32 Checksumming**: Superblocks and data chunks include CRC32 checksums calculated with polynomial `0xEDB88320`.
2. **Mount Failure Fallback**: If Sector 0 has corrupted magic or a failed CRC32, the VFS automatically reformats the partition and regenerates safe default files (`macro.txt`, `wifi_audit.log`).
3. **No Unbounded Malloc**: File descriptors and sector buffers are allocated statically in BSS.
