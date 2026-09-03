/**
 * @file memory_manager.h
 * @brief Custom static arena allocator (128KB in Internal SRAM).
 *
 * Search Complexity:
 * - Allocation (my_malloc): O(N) First-Fit linear traversal through block headers.
 * - Deallocation (my_free): O(1) constant-time bidirectional adjacent block coalescing.
 *
 * NOTE: Allocator is strictly for cooperative task contexts; allocation from ISR context is prohibited.
 */

#ifndef MK_MEMORY_MANAGER_H
#define MK_MEMORY_MANAGER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

mk_status_t mk_memory_init(void);
void mk_memory_reset(void);

void *my_malloc(size_t size);
void my_free(void *ptr);

size_t mk_memory_used(void);
size_t mk_memory_free(void);
size_t mk_memory_peak(void);

typedef struct {
    size_t total_capacity;
    size_t used_bytes;
    size_t free_bytes;
    size_t peak_used_bytes;
    uint32_t allocation_count;
    uint32_t free_count;
    uint32_t failed_allocations;
    uint32_t corruption_count;
    uint32_t double_free_attempts;
    uint32_t invalid_ptr_attempts;
} mk_mem_stats_t;

void mk_memory_get_stats(mk_mem_stats_t *out_stats);

#ifdef __cplusplus
}
#endif

#endif /* MK_MEMORY_MANAGER_H */
