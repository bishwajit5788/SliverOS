/* SliverOS cooperative scheduler: ESP-IDF/FreeRTOS supplies one underlying executive task;
 * SliverOS exclusively owns application dispatch. Applications never create FreeRTOS tasks. */
#include "scheduler.h"
#include "fault_manager.h"
#include "hal_timer.h"
#include <string.h>
#include <stdio.h>
#if defined(ESP_PLATFORM)
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static uint8_t s_rr_cursor = 0U;
static uint64_t s_last_tick_us = 0ULL;

static void scheduler_update_timebase(mk_kernel_t *kernel)
{
    const uint64_t now = hal_timer_get_us();
    if (s_last_tick_us == 0ULL) { s_last_tick_us = now; return; }
    const uint64_t elapsed = now - s_last_tick_us;
    const uint32_t ticks = (uint32_t)(elapsed / 1000ULL);
    if (ticks == 0U) return;
    s_last_tick_us += (uint64_t)ticks * 1000ULL;
    for (uint32_t i = 0U; i < ticks; ++i) mk_scheduler_tick(kernel);
}

mk_status_t mk_scheduler_init(mk_kernel_t *kernel)
{
    if (kernel == NULL) return MK_STATUS_INVALID_ARG;
    for (uint8_t i = 0U; i < MK_MAX_TASKS; ++i) {
        mk_tcb_t *tcb = &kernel->tasks[i];
        memset(tcb, 0, sizeof(*tcb));
        tcb->id = i; tcb->state = MK_TASK_STATE_UNUSED;
        tcb->priority = MK_TASK_PRIO_LOWEST;
        tcb->max_execution_us = MK_TASK_EXEC_BUDGET_US;
    }
    s_rr_cursor = 0U;
    s_last_tick_us = hal_timer_get_us();
    kernel->scheduler_iterations = 0U;
    return MK_STATUS_OK;
}

mk_status_t mk_task_register(mk_kernel_t *kernel, uint8_t task_id, const char *name,
                             mk_task_entry_t entry, void *context, uint8_t priority,
                             uint32_t period_ticks)
{
    if (kernel == NULL || task_id >= MK_MAX_TASKS || entry == NULL) {
        mk_fault_record_full(MK_FAULT_TASK_REGISTRATION_FAIL, MK_FAULT_SRC_SCHEDULER,
                             MK_FAULT_SEV_WARNING, task_id, 0U);
        return MK_STATUS_INVALID_ARG;
    }
    mk_tcb_t *tcb = &kernel->tasks[task_id];
    tcb->id = task_id; tcb->state = MK_TASK_STATE_READY;
    tcb->priority = (priority > MK_TASK_PRIO_LOWEST) ? MK_TASK_PRIO_LOWEST : priority;
    tcb->period_ticks = period_ticks; tcb->next_run_tick = kernel->tick;
    tcb->entry = entry; tcb->context = context;
    tcb->execution_count = 0U; tcb->fault_count = 0U;
    tcb->last_execution_us = 0U; tcb->worst_execution_us = 0U;
    tcb->max_execution_us = MK_TASK_EXEC_BUDGET_US;
    tcb->deadline_miss_count = 0U; tcb->overrun_count = 0U;
    if (name != NULL) {
        strncpy(tcb->name, name, MK_TASK_NAME_LEN - 1U);
        tcb->name[MK_TASK_NAME_LEN - 1U] = '\0';
    } else snprintf(tcb->name, sizeof(tcb->name), "task_%u", task_id);
    return MK_STATUS_OK;
}

mk_status_t mk_task_set_state(mk_kernel_t *kernel, uint8_t task_id, mk_task_state_t state)
{
    if (kernel == NULL || task_id >= MK_MAX_TASKS || state >= MK_TASK_STATE_COUNT)
        return MK_STATUS_INVALID_ARG;
    kernel->tasks[task_id].state = state;
    return MK_STATUS_OK;
}

void mk_scheduler_tick(mk_kernel_t *kernel)
{
    if (kernel == NULL) return;
    kernel->tick++;
    for (uint8_t i = 0U; i < MK_MAX_TASKS; ++i) {
        mk_tcb_t *tcb = &kernel->tasks[i];
        if (tcb->state == MK_TASK_STATE_SLEEPING && kernel->tick >= tcb->next_run_tick)
            tcb->state = MK_TASK_STATE_READY;
    }
}

mk_tcb_t *mk_scheduler_select_next(mk_kernel_t *kernel)
{
    if (kernel == NULL) return NULL;
    uint8_t best = 255U;
    for (uint8_t i = 0U; i < MK_MAX_TASKS; ++i) {
        mk_tcb_t *tcb = &kernel->tasks[i];
        if (tcb->state == MK_TASK_STATE_READY && kernel->tick >= tcb->next_run_tick && tcb->priority < best)
            best = tcb->priority;
    }
    if (best == 255U) return NULL;
    for (uint8_t off = 0U; off < MK_MAX_TASKS; ++off) {
        uint8_t idx = (uint8_t)((s_rr_cursor + off) % MK_MAX_TASKS);
        mk_tcb_t *candidate = &kernel->tasks[idx];
        if (candidate->state == MK_TASK_STATE_READY && kernel->tick >= candidate->next_run_tick && candidate->priority == best) {
            s_rr_cursor = (uint8_t)((idx + 1U) % MK_MAX_TASKS);
            return candidate;
        }
    }
    return NULL;
}

void mk_scheduler_run_iteration(mk_kernel_t *kernel)
{
    if (kernel == NULL) return;
    scheduler_update_timebase(kernel);
#if defined(ESP_PLATFORM)
    (void)esp_task_wdt_reset();
#endif
    mk_tcb_t *tcb = mk_scheduler_select_next(kernel);
    if (tcb != NULL && tcb->entry != NULL) {
        if (kernel->tick > tcb->next_run_tick) tcb->deadline_miss_count++;
        tcb->state = MK_TASK_STATE_RUNNING;
        const uint64_t start = hal_timer_get_us();
        tcb->entry(tcb->context);
        const uint64_t end = hal_timer_get_us();
        const uint32_t duration = (end >= start) ? (uint32_t)(end - start) : 0U;
        tcb->last_execution_us = duration;
        if (duration > tcb->worst_execution_us) tcb->worst_execution_us = duration;
        if (duration > tcb->max_execution_us) {
            ++tcb->overrun_count; ++tcb->fault_count;
            mk_fault_record_full(MK_FAULT_SCHEDULER_OVERRUN, MK_FAULT_SRC_SCHEDULER,
                                 MK_FAULT_SEV_WARNING, (uint32_t)tcb->id, duration);
        }
        ++tcb->execution_count;
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
        /* Never block the cooperative path. FreeRTOS only yields the underlying
         * executive thread; it does not perform application scheduling. */
#if defined(ESP_PLATFORM)
        taskYIELD();
#endif
    }
    ++kernel->scheduler_iterations;
}
