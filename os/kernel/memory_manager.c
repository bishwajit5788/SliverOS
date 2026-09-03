/**
 * @file memory_manager.c
 * @brief Deterministic 128KB static arena allocator in internal SRAM.
 */

#include "memory_manager.h"
#include "fault_manager.h"
#include <string.h>

#define MK_BLOCK_MAGIC_ALLOC    0x55AA55AAU
#define MK_BLOCK_MAGIC_FREE     0xAA55AA55U
#define MK_ALIGNMENT            8U
#define MK_ALIGN(x)             (((x) + (MK_ALIGNMENT - 1U)) & ~(MK_ALIGNMENT - 1U))
#define MK_MIN_PAYLOAD          8U

typedef struct mk_mem_block {
    uint32_t magic;                 /* Canary tag verifying block validity and state */
    uint32_t size;                  /* Usable payload size in bytes */
    struct mk_mem_block *prev;      /* Physically preceding block in arena */
    struct mk_mem_block *next;      /* Physically succeeding block in arena */
    uint32_t is_free;               /* 1 = free, 0 = allocated */
    uint32_t padding;               /* Ensures 8-byte alignment on 64-bit and 32-bit hosts */
} mk_mem_block_t;

_Static_assert(sizeof(mk_mem_block_t) % MK_ALIGNMENT == 0, "Header must be multiple of 8 bytes");

/* Static memory arena buffer - strictly placed in Internal SRAM */
static uint8_t s_arena_buffer[MK_ARENA_SIZE] __attribute__((aligned(MK_ALIGNMENT)));

static mk_mem_block_t *s_head_block = NULL;
static bool s_initialized = false;
static mk_mem_stats_t s_stats;

mk_status_t mk_memory_init(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
    s_stats.total_capacity = MK_ARENA_SIZE - sizeof(mk_mem_block_t);

    s_head_block = (mk_mem_block_t *)(void *)s_arena_buffer;
    s_head_block->magic = MK_BLOCK_MAGIC_FREE;
    s_head_block->size = (uint32_t)(MK_ARENA_SIZE - sizeof(mk_mem_block_t));
    s_head_block->prev = NULL;
    s_head_block->next = NULL;
    s_head_block->is_free = 1U;
    s_head_block->padding = 0U;

    s_stats.free_bytes = s_head_block->size;
    s_stats.used_bytes = 0U;
    s_stats.peak_used_bytes = 0U;

    s_initialized = true;
    return MK_STATUS_OK;
}

void mk_memory_reset(void)
{
    (void)mk_memory_init();
}

void *my_malloc(size_t size)
{
    if (!s_initialized) {
        if (mk_memory_init() != MK_STATUS_OK) {
            return NULL;
        }
    }

    if (size == 0 || size > (MK_ARENA_SIZE - sizeof(mk_mem_block_t))) {
        s_stats.failed_allocations++;
        return NULL;
    }

    const size_t aligned_size = MK_ALIGN(size);
    mk_mem_block_t *curr = s_head_block;

    /* First-Fit search traversal: O(N) */
    while (curr != NULL) {
        if (curr->magic != MK_BLOCK_MAGIC_FREE && curr->magic != MK_BLOCK_MAGIC_ALLOC) {
            /* Corrupted header detected */
            s_stats.corruption_count++;
            s_stats.failed_allocations++;
            mk_fault_record_full(MK_FAULT_MEMORY_CORRUPTION, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_CRITICAL, 0xBAD00001, (uint32_t)(uintptr_t)curr);
            return NULL;
        }

        if (curr->is_free && curr->size >= aligned_size) {
            const size_t excess = curr->size - aligned_size;

            /* Split block if excess capacity can accommodate another header + minimum payload */
            if (excess >= (sizeof(mk_mem_block_t) + MK_MIN_PAYLOAD)) {
                uint8_t *split_addr = (uint8_t *)curr + sizeof(mk_mem_block_t) + aligned_size;
                mk_mem_block_t *new_block = (mk_mem_block_t *)(void *)split_addr;

                new_block->magic = MK_BLOCK_MAGIC_FREE;
                new_block->size = (uint32_t)(excess - sizeof(mk_mem_block_t));
                new_block->is_free = 1U;
                new_block->padding = 0U;
                new_block->prev = curr;
                new_block->next = curr->next;

                if (curr->next != NULL) {
                    curr->next->prev = new_block;
                }
                curr->next = new_block;
                curr->size = (uint32_t)aligned_size;
            }

            curr->magic = MK_BLOCK_MAGIC_ALLOC;
            curr->is_free = 0U;

            s_stats.used_bytes += curr->size;
            if (s_stats.free_bytes >= (curr->size + sizeof(mk_mem_block_t))) {
                s_stats.free_bytes -= curr->size;
            } else {
                s_stats.free_bytes = 0;
            }

            if (s_stats.used_bytes > s_stats.peak_used_bytes) {
                s_stats.peak_used_bytes = s_stats.used_bytes;
            }
            s_stats.allocation_count++;

            return (void *)((uint8_t *)curr + sizeof(mk_mem_block_t));
        }

        curr = curr->next;
    }

    /* Allocation exhausted */
    s_stats.failed_allocations++;
    mk_fault_record_full(MK_FAULT_MEMORY_EXHAUSTED, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_WARNING, (uint32_t)aligned_size, 0U);
    return NULL;
}

void my_free(void *ptr)
{
    if (ptr == NULL || !s_initialized) {
        return;
    }

    const uintptr_t ptr_val = (uintptr_t)ptr;
    const uintptr_t arena_start = (uintptr_t)s_arena_buffer;
    const uintptr_t arena_end = arena_start + MK_ARENA_SIZE;

    if (ptr_val < (arena_start + sizeof(mk_mem_block_t)) || ptr_val >= arena_end) {
        s_stats.invalid_ptr_attempts++;
        mk_fault_record_full(MK_FAULT_MEMORY_INVALID_PTR, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_WARNING, 0xBAD00002, (uint32_t)ptr_val);
        return;
    }

    if ((ptr_val & (MK_ALIGNMENT - 1U)) != 0U) {
        s_stats.invalid_ptr_attempts++;
        mk_fault_record_full(MK_FAULT_MEMORY_INVALID_PTR, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_WARNING, 0xBAD00003, (uint32_t)ptr_val);
        return;
    }

    mk_mem_block_t *block = (mk_mem_block_t *)(void *)((uint8_t *)ptr - sizeof(mk_mem_block_t));

    if (block->magic == MK_BLOCK_MAGIC_FREE || block->is_free != 0U) {
        s_stats.double_free_attempts++;
        mk_fault_record_full(MK_FAULT_MEMORY_DOUBLE_FREE, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_WARNING, 0xBAD00004, (uint32_t)ptr_val);
        return;
    }

    if (block->magic != MK_BLOCK_MAGIC_ALLOC) {
        s_stats.corruption_count++;
        s_stats.invalid_ptr_attempts++;
        mk_fault_record_full(MK_FAULT_MEMORY_CORRUPTION, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_CRITICAL, 0xBAD00005, (uint32_t)ptr_val);
        return;
    }

    /* Free block */
    block->magic = MK_BLOCK_MAGIC_FREE;
    block->is_free = 1U;

    if (s_stats.used_bytes >= block->size) {
        s_stats.used_bytes -= block->size;
    } else {
        s_stats.used_bytes = 0U;
    }
    s_stats.free_bytes += block->size;
    s_stats.free_count++;

    /* Coalesce forward (next block) if adjacent block is free: O(1) */
    if (block->next != NULL && block->next->is_free != 0U) {
        mk_mem_block_t *next_block = block->next;
        block->size += (uint32_t)(sizeof(mk_mem_block_t) + next_block->size);
        block->next = next_block->next;
        if (next_block->next != NULL) {
            next_block->next->prev = block;
        }
        s_stats.free_bytes += sizeof(mk_mem_block_t);
    }

    /* Coalesce backward (prev block) if adjacent block is free: O(1) */
    if (block->prev != NULL && block->prev->is_free != 0U) {
        mk_mem_block_t *prev_block = block->prev;
        prev_block->size += (uint32_t)(sizeof(mk_mem_block_t) + block->size);
        prev_block->next = block->next;
        if (block->next != NULL) {
            block->next->prev = prev_block;
        }
        s_stats.free_bytes += sizeof(mk_mem_block_t);
    }
}

size_t mk_memory_used(void)
{
    return s_stats.used_bytes;
}

size_t mk_memory_free(void)
{
    return s_stats.free_bytes;
}

size_t mk_memory_peak(void)
{
    return s_stats.peak_used_bytes;
}

void mk_memory_get_stats(mk_mem_stats_t *out_stats)
{
    if (out_stats != NULL) {
        *out_stats = s_stats;
    }
}
