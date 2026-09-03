/**
 * @file fault_manager.h
 * @brief Static ring-buffer fault management subsystem with 10 diagnostic categories.
 */

#ifndef MK_FAULT_MANAGER_H
#define MK_FAULT_MANAGER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MK_FAULT_SRC_MEMORY = 0,
    MK_FAULT_SRC_SCHEDULER,
    MK_FAULT_SRC_STATE,
    MK_FAULT_SRC_VFS,
    MK_FAULT_SRC_HAL,
    MK_FAULT_SRC_WIFI,
    MK_FAULT_SRC_BLE,
    MK_FAULT_SRC_NETWORK,
    MK_FAULT_SRC_DISPLAY,
    MK_FAULT_SRC_SYSTEM,

    MK_FAULT_SRC_COUNT
} mk_fault_source_t;

typedef enum {
    MK_FAULT_SEV_INFO = 0,
    MK_FAULT_SEV_WARNING,
    MK_FAULT_SEV_RECOVERABLE,
    MK_FAULT_SEV_CRITICAL,
    MK_FAULT_SEV_PANIC
} mk_fault_severity_t;

typedef enum {
    MK_FAULT_NONE = 0,
    MK_FAULT_MEMORY_EXHAUSTED,
    MK_FAULT_MEMORY_CORRUPTION,
    MK_FAULT_MEMORY_DOUBLE_FREE,
    MK_FAULT_MEMORY_INVALID_PTR,
    MK_FAULT_STATE_INVALID_TRANSITION,
    MK_FAULT_TASK_STATE_INVALID,
    MK_FAULT_TASK_REGISTRATION_FAIL,
    MK_FAULT_SCHEDULER_OVERRUN,
    MK_FAULT_SCHEDULER_DEADLINE_MISS,
    MK_FAULT_VFS_FAIL,
    MK_FAULT_VFS_CORRUPTION_RECOVERED,
    MK_FAULT_HAL_FAIL,
    MK_FAULT_WATCHDOG_EVENT,
    MK_FAULT_APP_ERROR
} mk_fault_code_t;

typedef struct {
    uint32_t fault_id;
    uint32_t timestamp_ticks;
    mk_fault_code_t fault_code;
    mk_fault_source_t source;
    mk_fault_severity_t severity;
    uint32_t error_code;
    uint32_t context;
} mk_fault_record_t;

mk_status_t mk_fault_manager_init(void);

void mk_fault_record_full(mk_fault_code_t code, mk_fault_source_t source,
                          mk_fault_severity_t severity, uint32_t error_code, uint32_t context);

void mk_fault_record(mk_fault_code_t code, uint32_t source_id, uint32_t diag_code);

uint32_t mk_fault_get_total_count(void);
mk_status_t mk_fault_get_latest(mk_fault_record_t *out_record);
mk_status_t mk_fault_get_at(uint32_t offset_from_latest, mk_fault_record_t *out_record);
const char *mk_fault_source_str(mk_fault_source_t source);

#ifdef __cplusplus
}
#endif

#endif /* MK_FAULT_MANAGER_H */
