/**
 * @file memory_pool.h
 * @brief Fixed-size memory pool allocator for deterministic O(1) allocations.
 * Specifically designed for events, fault messages, and scheduler metadata.
 */

#ifndef MK_MEMORY_POOL_H
#define MK_MEMORY_POOL_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MK_POOL_CLASSES_COUNT   5U

typedef struct {
    size_t block_size;
    uint32_t total_blocks;
    uint32_t allocated_blocks;
    uint32_t peak_allocated;
    uint32_t allocation_failures;
} mk_pool_class_stats_t;

typedef struct {
    size_t total_capacity_bytes;
    size_t total_used_bytes;
    mk_pool_class_stats_t classes[MK_POOL_CLASSES_COUNT];
} mk_pool_stats_t;

/**
 * @brief Initialize all fixed-size pool classes in internal SRAM.
 */
mk_status_t mk_pool_init(void);

/**
 * @brief Allocate a fixed-size block from the smallest matching pool class.
 * Provides deterministic O(1) constant-time allocation.
 *
 * @param size Requested payload size in bytes (up to 256 bytes).
 * @return 8-byte aligned pointer, or NULL if size exceeds 256 or class is exhausted.
 */
void *mk_pool_alloc(size_t size);

/**
 * @brief Return a previously allocated pool block.
 * Provides deterministic O(1) constant-time deallocation.
 *
 * @param ptr Pointer previously returned by mk_pool_alloc.
 */
void mk_pool_free(void *ptr);

/**
 * @brief Retrieve snapshot of pool allocator metrics.
 */
void mk_pool_get_stats(mk_pool_stats_t *out_stats);

#ifdef __cplusplus
}
#endif

#endif /* MK_MEMORY_POOL_H */
