/**
 * @file test_vfs_block.c
 * @brief Unit tests for low-level VFS block storage and CRC verification.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "vfs_block.h"

void test_vfs_block(void)
{
    printf("[TEST] Starting VFS Block Storage Unit Tests...\n");

    assert(vfs_block_init() == MK_STATUS_OK);
    uint32_t count = vfs_block_get_sector_count();
    assert(count > 0);

    /* Test Sector Erase */
    assert(vfs_block_erase(10) == MK_STATUS_OK);
    uint8_t read_buf[VFS_SECTOR_SIZE];
    assert(vfs_block_read(10, read_buf, VFS_SECTOR_SIZE) == MK_STATUS_OK);
    for (size_t i = 0; i < VFS_SECTOR_SIZE; i++) {
        assert(read_buf[i] == 0xFF);
    }

    /* Test Write and Read */
    uint8_t write_buf[64];
    for (size_t i = 0; i < sizeof(write_buf); i++) {
        write_buf[i] = (uint8_t)(i ^ 0x5A);
    }
    assert(vfs_block_write(10, write_buf, sizeof(write_buf)) == MK_STATUS_OK);

    memset(read_buf, 0, sizeof(read_buf));
    assert(vfs_block_read(10, read_buf, sizeof(write_buf)) == MK_STATUS_OK);
    assert(memcmp(write_buf, read_buf, sizeof(write_buf)) == 0);

    /* Test CRC32 */
    const char *sample = "123456789";
    uint32_t crc = vfs_block_calc_crc32((const uint8_t *)sample, strlen(sample));
    assert(crc == 0xCBF43926U); /* Standard CRC32 check value for "123456789" */

    printf("[PASS] VFS Block Storage Unit Tests Passed Successfully.\n");
}
