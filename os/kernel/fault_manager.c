/**
 * @file fault_manager.c
 * @brief Fixed-size ring buffer implementation for fault auditing.
 */

#include "fault_manager.h"
#include <string.h>

static mk_fault_record_t s_fault_ring[MK_FAULT_RING_SIZE];
static uint32_t s_write_index = 0U;
static uint32_t s_total_faults = 0U;
static bool s_initialized = false;

mk_status_t mk_fault_manager_init(void)
{
    memset(s_fault_ring, 0, sizeof(s_fault_ring));
    s_write_index = 0U;
    s_total_faults = 0U;
    s_initialized = true;
    return MK_STATUS_OK;
}

void mk_fault_record_full(mk_fault_code_t code, mk_fault_source_t source,
                          mk_fault_severity_t severity, uint32_t error_code, uint32_t context)
{
    if (!s_initialized) {
        (void)mk_fault_manager_init();
    }

    uint32_t current_tick = 0U;
#if defined(MK_HOST_TEST)
    current_tick = s_total_faults;
#else
    extern uint32_t mk_kernel_get_tick(void) __attribute__((weak));
    if (mk_kernel_get_tick != NULL) {
        current_tick = mk_kernel_get_tick();
    }
#endif

    mk_fault_record_t *rec = &s_fault_ring[s_write_index % MK_FAULT_RING_SIZE];
    rec->fault_id = ++s_total_faults;
    rec->timestamp_ticks = current_tick;
    rec->fault_code = code;
    rec->source = (source < MK_FAULT_SRC_COUNT) ? source : MK_FAULT_SRC_SYSTEM;
    rec->severity = severity;
    rec->error_code = error_code;
    rec->context = context;

    s_write_index = (s_write_index + 1U) % MK_FAULT_RING_SIZE;
}

void mk_fault_record(mk_fault_code_t code, uint32_t source_id, uint32_t diag_code)
{
    mk_fault_source_t src = MK_FAULT_SRC_SYSTEM;
    if (source_id < (uint32_t)MK_FAULT_SRC_COUNT) {
        src = (mk_fault_source_t)source_id;
    }
    mk_fault_record_full(code, src, MK_FAULT_SEV_WARNING, diag_code, 0U);
}

uint32_t mk_fault_get_total_count(void)
{
    return s_total_faults;
}

mk_status_t mk_fault_get_latest(mk_fault_record_t *out_record)
{
    if (out_record == NULL || s_total_faults == 0U) {
        return MK_STATUS_NOT_FOUND;
    }

    uint32_t last_idx = (s_write_index == 0U) ? (MK_FAULT_RING_SIZE - 1U) : (s_write_index - 1U);
    *out_record = s_fault_ring[last_idx];
    return MK_STATUS_OK;
}

mk_status_t mk_fault_get_at(uint32_t offset_from_latest, mk_fault_record_t *out_record)
{
    if (out_record == NULL || s_total_faults == 0U) {
        return MK_STATUS_NOT_FOUND;
    }

    uint32_t valid_entries = (s_total_faults < MK_FAULT_RING_SIZE) ? s_total_faults : MK_FAULT_RING_SIZE;
    if (offset_from_latest >= valid_entries) {
        return MK_STATUS_NOT_FOUND;
    }

    uint32_t last_idx = (s_write_index == 0U) ? (MK_FAULT_RING_SIZE - 1U) : (s_write_index - 1U);
    uint32_t target_idx = (last_idx + MK_FAULT_RING_SIZE - offset_from_latest) % MK_FAULT_RING_SIZE;

    *out_record = s_fault_ring[target_idx];
    return MK_STATUS_OK;
}

const char *mk_fault_source_str(mk_fault_source_t source)
{
    switch (source) {
        case MK_FAULT_SRC_MEMORY:    return "MEMORY";
        case MK_FAULT_SRC_SCHEDULER: return "SCHEDULER";
        case MK_FAULT_SRC_STATE:     return "STATE";
        case MK_FAULT_SRC_VFS:       return "VFS";
        case MK_FAULT_SRC_HAL:       return "HAL";
        case MK_FAULT_SRC_WIFI:      return "WIFI";
        case MK_FAULT_SRC_BLE:       return "BLE";
        case MK_FAULT_SRC_NETWORK:   return "NETWORK";
        case MK_FAULT_SRC_DISPLAY:   return "DISPLAY";
        case MK_FAULT_SRC_SYSTEM:    return "SYSTEM";
        default:                     return "UNKNOWN";
    }
}
