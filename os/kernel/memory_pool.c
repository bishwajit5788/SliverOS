/**
 * @file memory_pool.c
 * @brief Deterministic O(1) fixed-size block pool allocator implementation.
 */

#include "memory_pool.h"
#include "fault_manager.h"
#include <string.h>
#if defined(ESP_PLATFORM)
#include "esp_attr.h"
#define MK_INTERNAL_DRAM DRAM_ATTR
#else
#define MK_INTERNAL_DRAM
#endif

#define MK_POOL_MAGIC_ALLOC 0x504F4F4CU
#define MK_POOL_MAGIC_FREE  0x46524545U

typedef struct mk_pool_node {
    uint32_t magic;
    uint8_t class_idx;
    uint8_t in_use;
    uint16_t reserved;
    struct mk_pool_node *next_free;
} mk_pool_node_t;

_Static_assert(sizeof(mk_pool_node_t) % 8 == 0, "Pool node header must be 8-byte aligned");

static const size_t s_block_sizes[MK_POOL_CLASSES_COUNT] = { 16U, 32U, 64U, 128U, 256U };
#define MK_POOL_0_COUNT 32U
#define MK_POOL_1_COUNT 32U
#define MK_POOL_2_COUNT 32U
#define MK_POOL_3_COUNT 16U
#define MK_POOL_4_COUNT 8U
static const uint32_t s_class_counts[MK_POOL_CLASSES_COUNT] = {
    MK_POOL_0_COUNT, MK_POOL_1_COUNT, MK_POOL_2_COUNT, MK_POOL_3_COUNT, MK_POOL_4_COUNT
};

/* Explicitly internal-DRAM-resident fixed pools. */
static MK_INTERNAL_DRAM uint8_t s_pool_buf_0[MK_POOL_0_COUNT * (sizeof(mk_pool_node_t) + 16U)] __attribute__((aligned(8)));
static MK_INTERNAL_DRAM uint8_t s_pool_buf_1[MK_POOL_1_COUNT * (sizeof(mk_pool_node_t) + 32U)] __attribute__((aligned(8)));
static MK_INTERNAL_DRAM uint8_t s_pool_buf_2[MK_POOL_2_COUNT * (sizeof(mk_pool_node_t) + 64U)] __attribute__((aligned(8)));
static MK_INTERNAL_DRAM uint8_t s_pool_buf_3[MK_POOL_3_COUNT * (sizeof(mk_pool_node_t) + 128U)] __attribute__((aligned(8)));
static MK_INTERNAL_DRAM uint8_t s_pool_buf_4[MK_POOL_4_COUNT * (sizeof(mk_pool_node_t) + 256U)] __attribute__((aligned(8)));

static uint8_t * const s_pool_buffers[MK_POOL_CLASSES_COUNT] = {
    s_pool_buf_0, s_pool_buf_1, s_pool_buf_2, s_pool_buf_3, s_pool_buf_4
};
static mk_pool_node_t *s_free_heads[MK_POOL_CLASSES_COUNT];
static mk_pool_stats_t s_stats;
static bool s_initialized = false;

mk_status_t mk_pool_init(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
    for (uint8_t c = 0; c < MK_POOL_CLASSES_COUNT; c++) {
        const size_t slot_size = sizeof(mk_pool_node_t) + s_block_sizes[c];
        const uint32_t count = s_class_counts[c];
        uint8_t *base = s_pool_buffers[c];
        s_stats.classes[c].block_size = s_block_sizes[c];
        s_stats.classes[c].total_blocks = count;
        s_stats.total_capacity_bytes += count * s_block_sizes[c];
        s_free_heads[c] = NULL;
        for (uint32_t i = 0; i < count; i++) {
            mk_pool_node_t *node = (mk_pool_node_t *)(void *)(base + (i * slot_size));
            node->magic = MK_POOL_MAGIC_FREE;
            node->class_idx = c;
            node->in_use = 0U;
            node->reserved = 0U;
            node->next_free = s_free_heads[c];
            s_free_heads[c] = node;
        }
    }
    s_initialized = true;
    return MK_STATUS_OK;
}

void *mk_pool_alloc(size_t size)
{
    if (!s_initialized && mk_pool_init() != MK_STATUS_OK) return NULL;
    if (size == 0 || size > s_block_sizes[MK_POOL_CLASSES_COUNT - 1]) return NULL;
    int target_class = -1;
    for (uint8_t c = 0; c < MK_POOL_CLASSES_COUNT; c++) {
        if (size <= s_block_sizes[c]) { target_class = c; break; }
    }
    if (target_class < 0) return NULL;
    mk_pool_node_t *node = s_free_heads[target_class];
    if (node == NULL) {
        s_stats.classes[target_class].allocation_failures++;
        mk_fault_record_full(MK_FAULT_MEMORY_EXHAUSTED, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_WARNING, (uint32_t)size, (uint32_t)target_class);
        return NULL;
    }
    s_free_heads[target_class] = node->next_free;
    node->next_free = NULL;
    node->magic = MK_POOL_MAGIC_ALLOC;
    node->in_use = 1U;
    s_stats.classes[target_class].allocated_blocks++;
    s_stats.total_used_bytes += s_block_sizes[target_class];
    if (s_stats.classes[target_class].allocated_blocks > s_stats.classes[target_class].peak_allocated)
        s_stats.classes[target_class].peak_allocated = s_stats.classes[target_class].allocated_blocks;
    return (void *)((uint8_t *)node + sizeof(mk_pool_node_t));
}

void mk_pool_free(void *ptr)
{
    if (ptr == NULL || !s_initialized) return;
    mk_pool_node_t *node = (mk_pool_node_t *)(void *)((uint8_t *)ptr - sizeof(mk_pool_node_t));
    if (node->magic != MK_POOL_MAGIC_ALLOC || node->in_use != 1U) {
        mk_fault_record_full(MK_FAULT_MEMORY_DOUBLE_FREE, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_WARNING, 0xBAD10001, (uint32_t)(uintptr_t)ptr);
        return;
    }
    const uint8_t c = node->class_idx;
    if (c >= MK_POOL_CLASSES_COUNT) {
        mk_fault_record_full(MK_FAULT_MEMORY_CORRUPTION, MK_FAULT_SRC_MEMORY, MK_FAULT_SEV_CRITICAL, 0xBAD10002, 0);
        return;
    }
    node->magic = MK_POOL_MAGIC_FREE;
    node->in_use = 0U;
    node->next_free = s_free_heads[c];
    s_free_heads[c] = node;
    if (s_stats.classes[c].allocated_blocks > 0) s_stats.classes[c].allocated_blocks--;
    if (s_stats.total_used_bytes >= s_block_sizes[c]) s_stats.total_used_bytes -= s_block_sizes[c];
}

void mk_pool_get_stats(mk_pool_stats_t *out_stats)
{
    if (out_stats != NULL) *out_stats = s_stats;
}
