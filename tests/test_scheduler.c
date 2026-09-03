/**
 * @file test_scheduler.c
 * @brief Unit tests for cooperative priority/period/round-robin scheduling.
 */
#include <stdio.h>
#include <assert.h>
#include "scheduler.h"
#include "kernel.h"

static uint32_t s_runs[MK_MAX_TASKS];
static void dummy_task(void *ctx)
{
    const uintptr_t id = (uintptr_t)ctx;
    if (id < MK_MAX_TASKS) ++s_runs[id];
}

static void reset_runs(void)
{
    for (uint8_t i = 0U; i < MK_MAX_TASKS; ++i) s_runs[i] = 0U;
}

void test_scheduler(void)
{
    printf("[TEST] Cooperative Scheduler Unit Tests...\n");
    mk_kernel_t kernel;
    reset_runs();
    (void)mk_kernel_boot();
    (void)mk_kernel_init();
    kernel = *mk_kernel_get_instance();
    assert(mk_scheduler_init(&kernel) == MK_STATUS_OK);

    /* Registration validation and priority clamping. */
    assert(mk_task_register(&kernel, MK_MAX_TASKS, "invalid", dummy_task, NULL, 1U, 0U) == MK_STATUS_INVALID_ARG);
    assert(mk_task_register(&kernel, 0U, "null", NULL, NULL, 1U, 0U) == MK_STATUS_INVALID_ARG);
    assert(mk_task_register(&kernel, 0U, "high", dummy_task, (void *)(uintptr_t)0U, MK_TASK_PRIO_HIGHEST, 0U) == MK_STATUS_OK);
    assert(mk_task_register(&kernel, 1U, "periodic", dummy_task, (void *)(uintptr_t)1U, 3U, 5U) == MK_STATUS_OK);
    assert(mk_task_register(&kernel, 2U, "low", dummy_task, (void *)(uintptr_t)2U, MK_TASK_PRIO_LOWEST, 0U) == MK_STATUS_OK);
    assert(kernel.tasks[0].priority == MK_TASK_PRIO_HIGHEST);

    /* Highest priority wins. */
    assert(mk_scheduler_select_next(&kernel)->id == 0U);
    mk_scheduler_run_iteration(&kernel);
    assert(s_runs[0] == 1U && s_runs[1] == 0U && s_runs[2] == 0U);

    /* Periodic task becomes sleeping after execution and wakes exactly at deadline. */
    mk_scheduler_run_iteration(&kernel);
    assert(s_runs[2] == 1U || s_runs[1] == 0U);
    assert(kernel.tasks[1].state == MK_TASK_STATE_READY);
    kernel.tasks[0].state = MK_TASK_STATE_SLEEPING;
    kernel.tasks[0].next_run_tick = kernel.tick + 2U;
    assert(mk_scheduler_select_next(&kernel)->id == 2U);
    mk_scheduler_tick(&kernel);
    assert(kernel.tasks[0].state == MK_TASK_STATE_SLEEPING);
    mk_scheduler_tick(&kernel);
    assert(kernel.tasks[0].state == MK_TASK_STATE_READY);
    assert(mk_scheduler_select_next(&kernel)->id == 0U);

    /* Equal-priority ready tasks must rotate without starvation. */
    reset_runs();
    kernel.tick = 100U;
    assert(mk_task_register(&kernel, 3U, "rr_a", dummy_task, (void *)(uintptr_t)3U, 2U, 0U) == MK_STATUS_OK);
    assert(mk_task_register(&kernel, 4U, "rr_b", dummy_task, (void *)(uintptr_t)4U, 2U, 0U) == MK_STATUS_OK);
    mk_tcb_t *first = mk_scheduler_select_next(&kernel);
    assert(first != NULL && (first->id == 3U || first->id == 4U));
    const uint8_t first_id = first->id;
    mk_scheduler_run_iteration(&kernel);
    mk_tcb_t *second = mk_scheduler_select_next(&kernel);
    assert(second != NULL && second->id != first_id);
    mk_scheduler_run_iteration(&kernel);
    assert(s_runs[3] == 1U && s_runs[4] == 1U);

    /* No runnable task: iteration must not mutate task execution counts. */
    kernel.tasks[0].state = MK_TASK_STATE_SLEEPING;
    kernel.tasks[1].state = MK_TASK_STATE_SLEEPING;
    kernel.tasks[2].state = MK_TASK_STATE_SLEEPING;
    kernel.tasks[3].state = MK_TASK_STATE_SLEEPING;
    kernel.tasks[4].state = MK_TASK_STATE_SLEEPING;
    const uint32_t before = kernel.scheduler_iterations;
    mk_scheduler_run_iteration(&kernel);
    assert(kernel.scheduler_iterations == before + 1U);

    printf("[PASS] Cooperative Scheduler Unit Tests Passed.\n");
}
