/**
 * @file scheduler.c
 * @brief Deterministic cooperative scheduler implementation with round-robin arbitration.
 */

#include "scheduler.h"
#include "fault_manager.h"
#include "hal_timer.h"
#include <string.h>
#include <stdio.h>

#if defined(ESP_PLATFORM)
#include "esp_task_wdt.h"
#endif

static uint8_t s_rr_cursor = 0U;

mk_status_t mk_scheduler_init(mk_kernel_t *kernel)
{
    if (kernel == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    for (uint8_t i = 0; i < MK_MAX_TASKS; i++) {
        mk_tcb_t *tcb = &kernel->tasks[i];
        memset(tcb, 0, sizeof(mk_tcb_t));
        tcb->id = i;
        tcb->state = MK_TASK_STATE_UNUSED;
        tcb->priority = MK_TASK_PRIO_LOWEST;
        tcb->max_execution_us = MK_TASK_EXEC_BUDGET_US;
    }

    s_rr_cursor = 0U;
    kernel->scheduler_iterations = 0U;
    return MK_STATUS_OK;
}

mk_status_t mk_task_register(mk_kernel_t *kernel, uint8_t task_id, const char *name,
                             mk_task_entry_t entry, void *context,
                             uint8_t priority, uint32_t period_ticks)
{
    if (kernel == NULL || task_id >= MK_MAX_TASKS || entry == NULL) {
        mk_fault_record_full(
            MK_FAULT_TASK_REGISTRATION_FAIL,
            MK_FAULT_SRC_SCHEDULER,
            MK_FAULT_SEV_WARNING,
            task_id, 0U
        );
        return MK_STATUS_INVALID_ARG;
    }

    mk_tcb_t *tcb = &kernel->tasks[task_id];
    tcb->id = task_id;
    tcb->state = MK_TASK_STATE_READY;
    tcb->priority = (priority > MK_TASK_PRIO_LOWEST) ? MK_TASK_PRIO_LOWEST : priority;
    tcb->period_ticks = period_ticks;
    tcb->next_run_tick = kernel->tick;
    tcb->entry = entry;
    tcb->context = context;
    tcb->execution_count = 0U;
    tcb->fault_count = 0U;
    tcb->last_execution_us = 0U;
    tcb->worst_execution_us = 0U;
    tcb->max_execution_us = MK_TASK_EXEC_BUDGET_US;
    tcb->deadline_miss_count = 0U;
    tcb->overrun_count = 0U;

    if (name != NULL) {
        strncpy(tcb->name, name, MK_TASK_NAME_LEN - 1U);
        tcb->name[MK_TASK_NAME_LEN - 1U] = '\0';
    } else {
        snprintf(tcb->name, sizeof(tcb->name), "task_%u", task_id);
    }

    return MK_STATUS_OK;
}

mk_status_t mk_task_set_state(mk_kernel_t *kernel, uint8_t task_id, mk_task_state_t state)
{
    if (kernel == NULL || task_id >= MK_MAX_TASKS || state >= MK_TASK_STATE_COUNT) {
        return MK_STATUS_INVALID_ARG;
    }

    kernel->tasks[task_id].state = state;
    return MK_STATUS_OK;
}

void mk_scheduler_tick(mk_kernel_t *kernel)
{
    if (kernel == NULL) {
        return;
    }

    kernel->tick++;

    /* Wake sleeping periodic tasks whose period elapsed */
    for (uint8_t i = 0; i < MK_MAX_TASKS; i++) {
        mk_tcb_t *tcb = &kernel->tasks[i];
        if (tcb->state == MK_TASK_STATE_SLEEPING) {
            if (kernel->tick >= tcb->next_run_tick) {
                tcb->state = MK_TASK_STATE_READY;
            }
        }
    }
}

mk_tcb_t *mk_scheduler_select_next(mk_kernel_t *kernel)
{
    if (kernel == NULL) {
        return NULL;
    }

    /* 1. Find the highest priority level with eligible ready tasks */
    uint8_t best_priority = 255U;

    for (uint8_t i = 0; i < MK_MAX_TASKS; i++) {
        mk_tcb_t *tcb = &kernel->tasks[i];
        if (tcb->state == MK_TASK_STATE_READY && kernel->tick >= tcb->next_run_tick) {
            if (tcb->priority < best_priority) {
                best_priority = tcb->priority;
            }
        }
    }

    if (best_priority == 255U) {
        return NULL; /* No eligible tasks */
    }

    /* 2. Equal-priority round-robin selection starting from s_rr_cursor */
    for (uint8_t offset = 0; offset < MK_MAX_TASKS; offset++) {
        uint8_t idx = (s_rr_cursor + offset) % MK_MAX_TASKS;
        mk_tcb_t *candidate = &kernel->tasks[idx];

        if (candidate->state == MK_TASK_STATE_READY &&
            kernel->tick >= candidate->next_run_tick &&
            candidate->priority == best_priority) {
            /* Advance round-robin cursor for fair arbitration */
            s_rr_cursor = (idx + 1U) % MK_MAX_TASKS;
            return candidate;
        }
    }

    return NULL;
}

void mk_scheduler_run_iteration(mk_kernel_t *kernel)
{
    if (kernel == NULL) {
        return;
    }

    /* Periodic watchdog service heartbeat */
#if defined(ESP_PLATFORM)
    (void)esp_task_wdt_reset();
#endif

    mk_tcb_t *tcb = mk_scheduler_select_next(kernel);

    if (tcb != NULL && tcb->entry != NULL) {
        /* Check if deadline was missed */
        if (kernel->tick > tcb->next_run_tick) {
            tcb->deadline_miss_count++;
        }

        tcb->state = MK_TASK_STATE_RUNNING;
        const uint64_t start_us = hal_timer_get_us();

        /* Execute bounded unit of work */
        tcb->entry(tcb->context);

        const uint64_t end_us = hal_timer_get_us();
        const uint32_t duration_us = (end_us >= start_us) ? (uint32_t)(end_us - start_us) : 0U;

        tcb->last_execution_us = duration_us;
        if (duration_us > tcb->worst_execution_us) {
            tcb->worst_execution_us = duration_us;
        }

        /* Detect task overrun against execution budget */
        if (duration_us > tcb->max_execution_us) {
            tcb->overrun_count++;
            tcb->fault_count++;
            mk_fault_record_full(
                MK_FAULT_SCHEDULER_OVERRUN,
                MK_FAULT_SRC_SCHEDULER,
                MK_FAULT_SEV_WARNING,
                (uint32_t)tcb->id,
                duration_us
            );
        }

        tcb->execution_count++;

        /* Update state and schedule next run */
        if (tcb->state == MK_TASK_STATE_RUNNING) {
            if (tcb->period_ticks > 0U) {
                tcb->next_run_tick = kernel->tick + tcb->period_ticks;
                tcb->state = MK_TASK_STATE_SLEEPING;
            } else {
                tcb->next_run_tick = kernel->tick;
                tcb->state = MK_TASK_STATE_READY;
            }
        }
    } else {
        /* Idle behavior: yield CPU slice */
        hal_timer_delay_ms(1U);
        mk_scheduler_tick(kernel);
    }

    kernel->scheduler_iterations++;
}
