/**
 * @file test_memory_manager.c
 * @brief Unit tests for static arena allocator.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "memory_manager.h"
#include "fault_manager.h"

void test_memory_manager(void)
{
    printf("[TEST] Starting Memory Manager Unit Tests...\n");

    mk_memory_reset();
    assert(mk_memory_used() == 0);
    assert(mk_memory_free() > 0);

    /* 1. Basic Allocation and Alignment */
    void *p1 = my_malloc(32);
    assert(p1 != NULL);
    assert(((uintptr_t)p1 % 8) == 0); /* 8-byte aligned */
    size_t used1 = mk_memory_used();
    assert(used1 >= 32);

    void *p2 = my_malloc(64);
    assert(p2 != NULL);
    assert(((uintptr_t)p2 % 8) == 0);
    size_t used2 = mk_memory_used();
    assert(used2 > used1);

    /* 2. Write and Read Data integrity */
    memset(p1, 0xAA, 32);
    memset(p2, 0xBB, 64);
    for (int i = 0; i < 32; i++) {
        assert(((uint8_t *)p1)[i] == 0xAA);
    }
    for (int i = 0; i < 64; i++) {
        assert(((uint8_t *)p2)[i] == 0xBB);
    }

    /* 3. Free and Coalescing */
    my_free(p1);
    assert(mk_memory_used() < used2);

    /* Allocate again to verify reuse of split/freed block */
    void *p3 = my_malloc(24);
    assert(p3 != NULL);
    assert(p3 == p1); /* Reuses first-fit block */

    my_free(p3);
    my_free(p2);

    /* After freeing both p3 and p2, arena should coalesce */
    assert(mk_memory_used() == 0);

    /* 4. Double-Free Protection */
    mk_mem_stats_t stats_before;
    mk_memory_get_stats(&stats_before);
    my_free(p2); /* Intentional double free */
    mk_mem_stats_t stats_after;
    mk_memory_get_stats(&stats_after);
    assert(stats_after.double_free_attempts == stats_before.double_free_attempts + 1);

    /* 5. Invalid Pointer Rejection */
    uint8_t bogus_stack_var = 0;
    my_free(&bogus_stack_var);
    mk_memory_get_stats(&stats_after);
    assert(stats_after.invalid_ptr_attempts > 0);

    /* Misaligned pointer rejection */
    my_free((void *)((uintptr_t)p1 + 3));

    /* 6. Allocation Exhaustion */
    void *giant = my_malloc(MK_ARENA_SIZE + 1024);
    assert(giant == NULL);

    printf("[PASS] Memory Manager Unit Tests Passed Successfully.\n");
}
