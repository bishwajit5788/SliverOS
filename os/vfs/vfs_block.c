/**
 * @file vfs_block.c
 * @brief Low-level NOR flash sector/block abstraction with power-loss recovery.
 */

#include "vfs_block.h"
#include <string.h>

#if defined(ESP_PLATFORM)
#include "esp_partition.h"
static const esp_partition_t *s_partition = NULL;
#else
#define SIM_SECTOR_COUNT 64U
static uint8_t s_sim_flash[SIM_SECTOR_COUNT * VFS_SECTOR_SIZE];
#endif

static uint32_t s_sector_count = 0U;
static bool s_initialized = false;

static uint32_t s_crc_table[256];
static bool s_crc_table_ready = false;

static void init_crc_table(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            c = (c & 1) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
        }
        s_crc_table[i] = c;
    }
    s_crc_table_ready = true;
}

uint32_t vfs_block_calc_crc32(const uint8_t *data, size_t length)
{
    if (!s_crc_table_ready) {
        init_crc_table();
    }
    uint32_t c = 0xFFFFFFFFU;
    for (size_t i = 0; i < length; i++) {
        c = s_crc_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFU;
}

mk_status_t vfs_block_init(void)
{
    if (!s_crc_table_ready) {
        init_crc_table();
    }

#if defined(ESP_PLATFORM)
    s_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
        VFS_PARTITION_LABEL
    );
    if (s_partition == NULL) {
        s_partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_ANY,
            VFS_PARTITION_LABEL
        );
    }
    if (s_partition == NULL) {
        return MK_STATUS_NOT_FOUND;
    }
    s_sector_count = s_partition->size / VFS_SECTOR_SIZE;
#else
    memset(s_sim_flash, 0xFF, sizeof(s_sim_flash));
    s_sector_count = SIM_SECTOR_COUNT;
#endif

    s_initialized = true;
    return MK_STATUS_OK;
}

uint32_t vfs_block_get_sector_count(void)
{
    return s_sector_count;
}

mk_status_t vfs_block_read(uint32_t sector_idx, void *buffer, size_t size)
{
    if (!s_initialized || buffer == NULL || sector_idx >= s_sector_count || size > VFS_SECTOR_SIZE) {
        return MK_STATUS_INVALID_ARG;
    }

#if defined(ESP_PLATFORM)
    const size_t offset = sector_idx * VFS_SECTOR_SIZE;
    if (esp_partition_read(s_partition, offset, buffer, size) != ESP_OK) {
        return MK_STATUS_IO_ERROR;
    }
#else
    memcpy(buffer, &s_sim_flash[sector_idx * VFS_SECTOR_SIZE], size);
#endif

    return MK_STATUS_OK;
}

mk_status_t vfs_block_write(uint32_t sector_idx, const void *buffer, size_t size)
{
    if (!s_initialized || buffer == NULL || sector_idx >= s_sector_count || size > VFS_SECTOR_SIZE) {
        return MK_STATUS_INVALID_ARG;
    }

#if defined(ESP_PLATFORM)
    const size_t offset = sector_idx * VFS_SECTOR_SIZE;
    if (esp_partition_write(s_partition, offset, buffer, size) != ESP_OK) {
        return MK_STATUS_IO_ERROR;
    }
#else
    memcpy(&s_sim_flash[sector_idx * VFS_SECTOR_SIZE], buffer, size);
#endif

    return MK_STATUS_OK;
}

mk_status_t vfs_block_erase(uint32_t sector_idx)
{
    if (!s_initialized || sector_idx >= s_sector_count) {
        return MK_STATUS_INVALID_ARG;
    }

#if defined(ESP_PLATFORM)
    const size_t offset = sector_idx * VFS_SECTOR_SIZE;
    if (esp_partition_erase_range(s_partition, offset, VFS_SECTOR_SIZE) != ESP_OK) {
        return MK_STATUS_IO_ERROR;
    }
#else
    memset(&s_sim_flash[sector_idx * VFS_SECTOR_SIZE], 0xFF, VFS_SECTOR_SIZE);
#endif

    return MK_STATUS_OK;
}

mk_status_t vfs_record_format(void *out_buf, size_t max_buf_len, uint16_t seq,
                              const void *payload, size_t payload_len, size_t *out_total_len)
{
    if (out_buf == NULL || payload == NULL || out_total_len == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    const size_t total_len = sizeof(vfs_record_header_t) + payload_len + sizeof(uint32_t);
    if (total_len > max_buf_len) {
        return MK_STATUS_NO_MEMORY;
    }

    vfs_record_header_t *hdr = (vfs_record_header_t *)out_buf;
    hdr->magic = VFS_RECORD_MAGIC;
    hdr->version = 1U;
    hdr->sequence = seq;
    hdr->payload_len = (uint32_t)payload_len;
    hdr->payload_crc32 = vfs_block_calc_crc32((const uint8_t *)payload, payload_len);

    uint8_t *payload_dst = (uint8_t *)out_buf + sizeof(vfs_record_header_t);
    memcpy(payload_dst, payload, payload_len);

    /* Write 4-byte commit marker at end of payload */
    uint32_t *commit_ptr = (uint32_t *)(void *)(payload_dst + payload_len);
    *commit_ptr = VFS_RECORD_COMMIT_MAGIC;

    *out_total_len = total_len;
    return MK_STATUS_OK;
}

mk_status_t vfs_record_validate(const void *buffer, size_t buf_len, size_t *out_payload_len)
{
    if (buffer == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    if (buf_len < (sizeof(vfs_record_header_t) + sizeof(uint32_t))) {
        return MK_STATUS_CORRUPTED;
    }

    const vfs_record_header_t *hdr = (const vfs_record_header_t *)buffer;
    if (hdr->magic != VFS_RECORD_MAGIC || hdr->version != 1U) {
        return MK_STATUS_CORRUPTED;
    }

    const size_t expected_total = sizeof(vfs_record_header_t) + hdr->payload_len + sizeof(uint32_t);
    if (expected_total > buf_len) {
        return MK_STATUS_CORRUPTED;
    }

    /* Verify payload CRC32 */
    const uint8_t *payload = (const uint8_t *)buffer + sizeof(vfs_record_header_t);
    uint32_t calc_crc = vfs_block_calc_crc32(payload, hdr->payload_len);
    if (calc_crc != hdr->payload_crc32) {
        return MK_STATUS_CORRUPTED;
    }

    /* Verify commit marker */
    const uint32_t *commit_ptr = (const uint32_t *)(const void *)(payload + hdr->payload_len);
    if (*commit_ptr != VFS_RECORD_COMMIT_MAGIC) {
        return MK_STATUS_CORRUPTED; /* Interrupted write without commit marker */
    }

    if (out_payload_len != NULL) {
        *out_payload_len = hdr->payload_len;
    }

    return MK_STATUS_OK;
}
