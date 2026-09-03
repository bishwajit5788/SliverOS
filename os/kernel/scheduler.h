/**
 * @file scheduler.h
 * @brief Deterministic non-preemptive cooperative scheduler with round-robin arbitration.
 *
 * Scheduling Model:
 * 1. Priority Selection (0 = highest, 7 = lowest).
 * 2. Period Eligibility: periodic tasks execute only when kernel_tick >= next_run_tick.
 * 3. Equal-Priority Round-Robin: circular pointer advances across tasks with identical priority.
 *
 * Note on Detection vs Preemption:
 * In a cooperative scheduler, tasks MUST voluntarily yield. Execution duration tracking
 * and budget enforcement DETECT long-running runaway tasks and log faults; they CANNOT
 * forcibly preempt them. Runaway task recovery is delegated to the hardware Watchdog (TWDT).
 */

#ifndef MK_SCHEDULER_H
#define MK_SCHEDULER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MK_TASK_EXEC_BUDGET_US   25000U /* 25 ms execution budget */

mk_status_t mk_scheduler_init(mk_kernel_t *kernel);

mk_status_t mk_task_register(mk_kernel_t *kernel, uint8_t task_id, const char *name,
                             mk_task_entry_t entry, void *context,
                             uint8_t priority, uint32_t period_ticks);

mk_status_t mk_task_set_state(mk_kernel_t *kernel, uint8_t task_id, mk_task_state_t state);
void mk_scheduler_tick(mk_kernel_t *kernel);
mk_tcb_t *mk_scheduler_select_next(mk_kernel_t *kernel);
void mk_scheduler_run_iteration(mk_kernel_t *kernel);

#ifdef __cplusplus
}
#endif

#endif /* MK_SCHEDULER_H */
