/**
 * @file kernel_types.h
 * @brief Core type definitions, enums, structures, and static assertions for MicroKernel OS.
 * Grounded for ESP32-S3-DevKitC-1 with strict internal SRAM and PSRAM architectural boundaries.
 */

#ifndef MK_KERNEL_TYPES_H
#define MK_KERNEL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum configuration constants */
#define MK_MAX_TASKS        8U
#define MK_TASK_NAME_LEN    16U
#define MK_ARENA_SIZE       (128U * 1024U)  /* Strictly placed in internal SRAM */
#define MK_FAULT_RING_SIZE  32U
#define MK_EVENT_QUEUE_SIZE 64U

/* Status / Error Codes */
typedef enum {
    MK_STATUS_OK                = 0,
    MK_STATUS_ERROR             = -1,
    MK_STATUS_INVALID_ARG       = -2,
    MK_STATUS_NO_MEMORY         = -3,
    MK_STATUS_INVALID_STATE     = -4,
    MK_STATUS_TIMEOUT           = -5,
    MK_STATUS_NOT_FOUND         = -6,
    MK_STATUS_BUSY              = -7,
    MK_STATUS_OVERRUN           = -8,
    MK_STATUS_IO_ERROR          = -9,
    MK_STATUS_CORRUPTED         = -10,
    MK_STATUS_QUEUE_FULL        = -11,
    MK_STATUS_QUEUE_EMPTY       = -12
} mk_status_t;

/* Kernel Lifecycle States */
typedef enum {
    MK_KERNEL_STATE_RESET = 0,
    MK_KERNEL_STATE_INIT,
    MK_KERNEL_STATE_READY,
    MK_KERNEL_STATE_RUNNING,
    MK_KERNEL_STATE_SLEEPING,
    MK_KERNEL_STATE_FAULT,
    MK_KERNEL_STATE_SHUTDOWN,

    MK_KERNEL_STATE_COUNT
} mk_kernel_state_t;

/* Task States */
typedef enum {
    MK_TASK_STATE_UNUSED = 0,
    MK_TASK_STATE_READY,
    MK_TASK_STATE_RUNNING,
    MK_TASK_STATE_BLOCKED,
    MK_TASK_STATE_SLEEPING,
    MK_TASK_STATE_TERMINATED,
    MK_TASK_STATE_FAULT,

    MK_TASK_STATE_COUNT
} mk_task_state_t;

/* Application States */
typedef enum {
    MK_APP_STATE_OFF = 0,
    MK_APP_STATE_INIT,
    MK_APP_STATE_READY,
    MK_APP_STATE_ACTIVE,
    MK_APP_STATE_STOPPING,
    MK_APP_STATE_ERROR,

    MK_APP_STATE_COUNT
} mk_app_state_t;

/* Exactly Four Applications */
typedef enum {
    MK_APP_BLE_HID = 0,
    MK_APP_WIFI_DIAGNOSTICS,
    MK_APP_NETWORK_DIAGNOSTICS,
    MK_APP_RETRO_GAMES,

    MK_APP_COUNT
} mk_app_id_t;

/* Priority levels (0 = highest, 7 = lowest) */
#define MK_TASK_PRIO_HIGHEST    0U
#define MK_TASK_PRIO_HIGH       2U
#define MK_TASK_PRIO_MEDIUM     4U
#define MK_TASK_PRIO_LOW        6U
#define MK_TASK_PRIO_LOWEST     7U

/* Task Entry Point Signature */
typedef void (*mk_task_entry_t)(void *context);

/* Task Control Block (TCB) - Stored in Internal SRAM */
typedef struct {
    uint8_t id;
    mk_task_state_t state;
    uint8_t priority;
    uint8_t padding;

    uint32_t period_ticks;
    uint32_t next_run_tick;

    mk_task_entry_t entry;
    void *context;

    /* Execution and Telemetry Metrics */
    uint32_t execution_count;
    uint32_t fault_count;
    uint32_t max_execution_us;
    uint32_t last_execution_us;
    uint32_t worst_execution_us;
    uint32_t deadline_miss_count;
    uint32_t overrun_count;

    char name[MK_TASK_NAME_LEN];
} mk_tcb_t;

/* Application Control Block */
typedef struct {
    mk_app_id_t id;
    mk_app_state_t state;
    uint8_t task_id;
    uint8_t padding[5];
    const char *name;
    uint32_t runs_completed;
    uint32_t error_count;
} mk_app_control_t;

/* Kernel Event Types */
typedef enum {
    MK_EVENT_NONE = 0,
    MK_EVENT_TIMER,
    MK_EVENT_GPIO,
    MK_EVENT_WIFI_FRAME,
    MK_EVENT_BLE_CONNECTED,
    MK_EVENT_BLE_DISCONNECTED,
    MK_EVENT_VFS,
    MK_EVENT_NETWORK,
    MK_EVENT_DISPLAY,
    MK_EVENT_FAULT,

    MK_EVENT_COUNT
} mk_event_type_t;

/* Fixed-size Event Record */
typedef struct {
    mk_event_type_t type;
    uint32_t timestamp_us;
    uint32_t source_id;
    uint32_t param1;
    uint32_t param2;
    void *payload;
} mk_event_t;

/* Kernel Control Block - Stored in Internal SRAM */
typedef struct {
    mk_kernel_state_t state;
    uint32_t tick;
    uint32_t scheduler_iterations;
    uint8_t active_app;
    uint8_t padding[3];

    mk_tcb_t tasks[MK_MAX_TASKS];
    mk_app_control_t apps[MK_APP_COUNT];
} mk_kernel_t;

/* Hardware & Firmware Identity Diagnostic Structure */
typedef struct {
    char product[32];
    char version[16];
    char build_id[32];
    char git_revision[40];
    char chip_family[16];
    uint32_t chip_revision;
    uint32_t flash_size_bytes;
    uint32_t psram_size_bytes;
    size_t internal_sram_total;
    size_t internal_sram_free;
    size_t psram_total;
    size_t psram_free;
} mk_diag_identity_t;

/* Compile-time Static Assertions */
_Static_assert(MK_MAX_TASKS == 8U, "MicroKernel requirement: exactly 8 maximum tasks");
_Static_assert(MK_APP_COUNT == 4U, "MicroKernel requirement: exactly 4 application IDs");
_Static_assert(MK_ARENA_SIZE == (128U * 1024U), "MicroKernel requirement: 128KB static arena in internal SRAM");
_Static_assert(sizeof(mk_tcb_t) % 4 == 0, "TCB structure must be 4-byte aligned");
_Static_assert(sizeof(mk_kernel_t) % 4 == 0, "Kernel control block must be 4-byte aligned");

#ifdef __cplusplus
}
#endif

#endif /* MK_KERNEL_TYPES_H */
