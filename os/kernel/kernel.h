/**
 * @file kernel.h
 * @brief Core executive coordinator and lifecycle interfaces for MicroKernel OS.
 */

#ifndef MK_KERNEL_H
#define MK_KERNEL_H

#include "kernel_types.h"
#include "scheduler.h"
#include "state_machine.h"
#include "memory_manager.h"
#include "fault_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MK_OS_VERSION       "1.0.0"
#define MK_OS_BUILD_ID      "MK-ESP32-20260904"

/**
 * @brief Retrieve singleton kernel control block instance.
 */
mk_kernel_t *mk_kernel_get_instance(void);

/**
 * @brief Primary bootloader transition: moves state from RESET to INIT.
 */
mk_status_t mk_kernel_boot(void);

/**
 * @brief Executive subsystem initialization: prepares scheduler, apps, and control blocks.
 */
mk_status_t mk_kernel_init(void);

/**
 * @brief Register an application into the kernel control block.
 */
mk_status_t mk_kernel_register_app(mk_app_id_t app_id, const char *name, uint8_t task_id);

/**
 * @brief Retrieve control block of a registered application.
 */
mk_app_control_t *mk_kernel_get_app(mk_app_id_t app_id);

/**
 * @brief Main execution loop of the cooperative microkernel executive.
 * Transitions kernel from READY to RUNNING and runs cooperative scheduling iterations.
 */
mk_status_t mk_kernel_run(void);

/**
 * @brief Graceful shutdown procedure.
 */
void mk_kernel_shutdown(void);

/**
 * @brief Current system tick counter.
 */
uint32_t mk_kernel_get_tick(void);

/**
 * @brief Populate startup diagnostic identity structure.
 */
void mk_kernel_get_diag_identity(mk_diag_identity_t *out_identity);

#ifdef __cplusplus
}
#endif

#endif /* MK_KERNEL_H */
