/**
 * @file test_scheduler.c
 * @brief Unit tests for cooperative scheduler with round-robin arbitration.
 */

#include <stdio.h>
#include <assert.h>
#include "scheduler.h"
#include "kernel.h"

static uint32_t s_task_a_runs = 0;
static uint32_t s_task_b_runs = 0;
static uint32_t s_task_c_runs = 0;

static void dummy_task_a(void *ctx)
{
    (void)ctx;
    s_task_a_runs++;
}

static void dummy_task_b(void *ctx)
{
    (void)ctx;
    s_task_b_runs++;
}

static void dummy_task_c(void *ctx)
{
    (void)ctx;
    s_task_c_runs++;
}

void test_scheduler(void)
{
    printf("[TEST] Starting Cooperative Scheduler Unit Tests...\n");

    mk_kernel_t kernel;
    (void)mk_kernel_boot();
    (void)mk_kernel_init();

    kernel.tick = 0;
    assert(mk_scheduler_init(&kernel) == MK_STATUS_OK);

    /* 1. Invalid Task Registration */
    assert(mk_task_register(&kernel, MK_MAX_TASKS, "invalid", dummy_task_a, NULL, 1, 0) == MK_STATUS_INVALID_ARG);
    assert(mk_task_register(&kernel, 0, "null_entry", NULL, NULL, 1, 0) == MK_STATUS_INVALID_ARG);

    /* 2. Valid Registration: Task 0 (priority 4) and Task 1 (priority 1) */
    s_task_a_runs = 0;
    s_task_b_runs = 0;
    assert(mk_task_register(&kernel, 0, "task_low", dummy_task_a, NULL, 4, 1) == MK_STATUS_OK);
    assert(mk_task_register(&kernel, 1, "task_high", dummy_task_b, NULL, 1, 5) == MK_STATUS_OK);

    /* 3. Priority Selection: Task 1 should run first because priority 1 < priority 4 */
    mk_tcb_t *selected = mk_scheduler_select_next(&kernel);
    assert(selected != NULL);
    assert(selected->id == 1);

    /* Run iteration */
    mk_scheduler_run_iteration(&kernel);
    assert(s_task_b_runs == 1);
    assert(s_task_a_runs == 0);
    assert(kernel.tasks[1].execution_count == 1);

    /* Task 1 now has period 5, so next run should pick Task 0 */
    selected = mk_scheduler_select_next(&kernel);
    assert(selected != NULL);
    assert(selected->id == 0);

    mk_scheduler_run_iteration(&kernel);
    assert(s_task_a_runs == 1);

    /* Advance tick by 5 */
    for (int i = 0; i < 5; i++) {
        mk_scheduler_tick(&kernel);
    }

    /* Task 1 should wake up and run again */
    selected = mk_scheduler_select_next(&kernel);
    assert(selected != NULL);
    assert(selected->id == 1);

    /* 4. Equal-Priority Round Robin Verification */
    s_task_b_runs = 0;
    s_task_c_runs = 0;
    assert(mk_task_register(&kernel, 2, "task_eq1", dummy_task_b, NULL, 2, 0) == MK_STATUS_OK);
    assert(mk_task_register(&kernel, 3, "task_eq2", dummy_task_c, NULL, 2, 0) == MK_STATUS_OK);

    /* Both tasks 2 and 3 are ready with equal priority 2. They should alternate in round-robin */
    mk_tcb_t *rr1 = mk_scheduler_select_next(&kernel);
    assert(rr1 != NULL);
    mk_scheduler_run_iteration(&kernel);

    mk_tcb_t *rr2 = mk_scheduler_select_next(&kernel);
    assert(rr2 != NULL);
    assert(rr1->id != rr2->id); /* Confirms alternating round-robin arbitration */

    printf("[PASS] Cooperative Scheduler Unit Tests Passed Successfully.\n");
}
