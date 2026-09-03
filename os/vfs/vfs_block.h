/**
 * @file vfs_block.h
 * @brief Low-level NOR flash sector abstraction with power-loss safe record structures.
 *
 * Hardware Characteristics:
 * - Erase Block Size: 4096 bytes (4KB NOR sector)
 * - Sector Alignment: Transactions must be aligned to 4KB boundaries
 * - Write Granularity: 4 bytes (32-bit words). NOR flash allows 1->0 bit flips;
 *   reversing bits 0->1 requires an erase command.
 * - Endurance: Typical NOR flash sectors sustain ~100,000 erase cycles.
 * - Power-Loss Recovery: Append records use explicit commit markers (0x55AA55AA)
 *   and CRC32 checksums. Incomplete writes lacking commit markers are safely ignored on mount.
 */

#ifndef MK_VFS_BLOCK_H
#define MK_VFS_BLOCK_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_SECTOR_SIZE         4096U
#define VFS_BLOCK_MAGIC         0x4F534653U /* "OSFS" */
#define VFS_PARTITION_LABEL     "osfs"

#define VFS_RECORD_MAGIC        0x52454331U /* "REC1" */
#define VFS_RECORD_COMMIT_MAGIC 0x55AA55AAU /* Commit Marker */

typedef enum {
    VFS_SECTOR_FREE = 0xFFFFFFFFU,
    VFS_SECTOR_ACTIVE = 0xA5A5A5A5U,
    VFS_SECTOR_OBSOLETE = 0x5A5A5A5AU
} vfs_sector_state_t;

/**
 * @brief Power-loss safe record header.
 */
typedef struct {
    uint32_t magic;         /* VFS_RECORD_MAGIC */
    uint16_t version;       /* Record version (1) */
    uint16_t sequence;      /* Monotonically increasing sequence number */
    uint32_t payload_len;   /* Usable payload size */
    uint32_t payload_crc32; /* CRC32 across payload bytes */
} vfs_record_header_t;

_Static_assert(sizeof(vfs_record_header_t) % 4 == 0, "Record header must be 4-byte aligned");

mk_status_t vfs_block_init(void);
uint32_t vfs_block_get_sector_count(void);
mk_status_t vfs_block_read(uint32_t sector_idx, void *buffer, size_t size);
mk_status_t vfs_block_write(uint32_t sector_idx, const void *buffer, size_t size);
mk_status_t vfs_block_erase(uint32_t sector_idx);
uint32_t vfs_block_calc_crc32(const uint8_t *data, size_t length);

/**
 * @brief Validate a power-loss safe record from raw buffer.
 * Verifies magic, bounds, payload CRC32, and commit marker.
 *
 * @param buffer Buffer containing raw record.
 * @param buf_len Length of buffer.
 * @param out_payload_len Pointer to store payload length if valid.
 * @return MK_STATUS_OK if completely committed and valid; error code if corrupt or incomplete.
 */
mk_status_t vfs_record_validate(const void *buffer, size_t buf_len, size_t *out_payload_len);

/**
 * @brief Write a power-loss safe record into a buffer with a trailing commit marker.
 */
mk_status_t vfs_record_format(void *out_buf, size_t max_buf_len, uint16_t seq,
                              const void *payload, size_t payload_len, size_t *out_total_len);

#ifdef __cplusplus
}
#endif

#endif /* MK_VFS_BLOCK_H */
