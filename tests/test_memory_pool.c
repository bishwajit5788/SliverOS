/**
 * @file test_memory_pool.c
 * @brief Unit tests for the fixed-size memory pool allocator.
 */

#include "memory_pool.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void run_memory_pool_tests(void)
{
    printf("[TEST] Starting Fixed Memory Pool Unit Tests...\n");

    mk_status_t status = mk_pool_init();
    assert(status == MK_STATUS_OK);

    mk_pool_stats_t stats;
    mk_pool_get_stats(&stats);
    assert(stats.total_capacity_bytes > 0);
    assert(stats.total_used_bytes == 0);

    /* 1. Allocate from 16-byte class */
    void *p16_a = mk_pool_alloc(12);
    assert(p16_a != NULL);
    assert(((uintptr_t)p16_a % 8) == 0); /* 8-byte alignment */

    void *p16_b = mk_pool_alloc(16);
    assert(p16_b != NULL);
    assert(p16_a != p16_b);

    mk_pool_get_stats(&stats);
    assert(stats.classes[0].allocated_blocks == 2);
    assert(stats.total_used_bytes == 32);

    /* 2. Free p16_a and allocate again to verify reuse */
    mk_pool_free(p16_a);
    mk_pool_get_stats(&stats);
    assert(stats.classes[0].allocated_blocks == 1);

    void *p16_c = mk_pool_alloc(10);
    assert(p16_c == p16_a); /* Head of free list reused */

    /* 3. Allocate from other classes */
    void *p32 = mk_pool_alloc(28);
    assert(p32 != NULL);

    void *p64 = mk_pool_alloc(60);
    assert(p64 != NULL);

    void *p128 = mk_pool_alloc(120);
    assert(p128 != NULL);

    void *p256 = mk_pool_alloc(250);
    assert(p256 != NULL);

    /* Oversized allocation must fail */
    void *p_oversized = mk_pool_alloc(300);
    assert(p_oversized == NULL);

    /* Clean up */
    mk_pool_free(p16_b);
    mk_pool_free(p16_c);
    mk_pool_free(p32);
    mk_pool_free(p64);
    mk_pool_free(p128);
    mk_pool_free(p256);

    mk_pool_get_stats(&stats);
    assert(stats.total_used_bytes == 0);

    /* Double free protection */
    mk_pool_free(p256);
    mk_pool_get_stats(&stats);
    assert(stats.total_used_bytes == 0);

    printf("[PASS] Fixed Memory Pool Unit Tests Passed Successfully.\n");
}
