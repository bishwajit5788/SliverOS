/**
 * @file test_vfs_recovery.c
 * @brief Unit tests for power-loss safe storage and commit marker recovery.
 */

#include "vfs_block.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void run_vfs_recovery_tests(void)
{
    printf("[TEST] Starting VFS Power-Loss Safe Recovery Unit Tests...\n");

    uint8_t buffer[512];
    const char *test_msg = "POWER_LOSS_SAFE_PAYLOAD_TEST_DATA";
    size_t test_len = strlen(test_msg);
    size_t total_len = 0;

    /* 1. Format valid record */
    mk_status_t status = vfs_record_format(buffer, sizeof(buffer), 1U, test_msg, test_len, &total_len);
    assert(status == MK_STATUS_OK);
    assert(total_len > test_len);

    /* 2. Validate clean record */
    size_t valid_payload_len = 0;
    status = vfs_record_validate(buffer, total_len, &valid_payload_len);
    assert(status == MK_STATUS_OK);
    assert(valid_payload_len == test_len);

    /* 3. Simulate power loss during write (missing trailing commit marker) */
    uint8_t interrupted_buf[512];
    memcpy(interrupted_buf, buffer, total_len);
    /* Incomplete write: zero out the last 4 bytes (commit marker) */
    memset(&interrupted_buf[total_len - 4], 0, 4);

    status = vfs_record_validate(interrupted_buf, total_len, &valid_payload_len);
    assert(status == MK_STATUS_CORRUPTED); /* Must reject incomplete write */

    /* 4. Simulate bit rot / corruption in payload */
    uint8_t corrupted_buf[512];
    memcpy(corrupted_buf, buffer, total_len);
    /* Flip a bit in the payload area */
    corrupted_buf[sizeof(vfs_record_header_t) + 2] ^= 0xFF;

    status = vfs_record_validate(corrupted_buf, total_len, &valid_payload_len);
    assert(status == MK_STATUS_CORRUPTED); /* Must reject CRC mismatch */

    /* 5. Truncated buffer check */
    status = vfs_record_validate(buffer, sizeof(vfs_record_header_t) - 2, &valid_payload_len);
    assert(status == MK_STATUS_CORRUPTED);

    printf("[PASS] VFS Power-Loss Safe Recovery Unit Tests Passed Successfully.\n");
}
