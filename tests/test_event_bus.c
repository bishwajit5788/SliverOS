/**
 * @file test_event_bus.c
 * @brief Unit tests for the kernel event bus subsystem.
 */

#include "event_bus.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void run_event_bus_tests(void)
{
    printf("[TEST] Starting Kernel Event Bus Unit Tests...\n");

    mk_status_t status = mk_event_bus_init();
    assert(status == MK_STATUS_OK);
    assert(mk_event_bus_pending_count() == 0);

    /* 1. Empty queue pop */
    mk_event_t evt;
    status = mk_event_bus_pop(&evt);
    assert(status == MK_STATUS_QUEUE_EMPTY);

    /* 2. Enqueue multiple events */
    mk_event_t evt1 = {
        .type = MK_EVENT_TIMER,
        .timestamp_us = 1000U,
        .source_id = 1U,
        .param1 = 42U,
        .param2 = 0U,
        .payload = NULL
    };
    status = mk_event_bus_post(&evt1);
    assert(status == MK_STATUS_OK);
    assert(mk_event_bus_pending_count() == 1);

    mk_event_t evt2 = {
        .type = MK_EVENT_GPIO,
        .timestamp_us = 1050U,
        .source_id = 2U,
        .param1 = 0x01U,
        .param2 = 0U,
        .payload = NULL
    };
    status = mk_event_bus_post(&evt2);
    assert(status == MK_STATUS_OK);
    assert(mk_event_bus_pending_count() == 2);

    /* 3. Peek check */
    mk_event_t peek_evt;
    status = mk_event_bus_peek(&peek_evt);
    assert(status == MK_STATUS_OK);
    assert(peek_evt.type == MK_EVENT_TIMER);
    assert(peek_evt.param1 == 42U);
    assert(mk_event_bus_pending_count() == 2);

    /* 4. FIFO Pop check */
    mk_event_t pop1;
    status = mk_event_bus_pop(&pop1);
    assert(status == MK_STATUS_OK);
    assert(pop1.type == MK_EVENT_TIMER);
    assert(pop1.param1 == 42U);
    assert(mk_event_bus_pending_count() == 1);

    mk_event_t pop2;
    status = mk_event_bus_pop(&pop2);
    assert(status == MK_STATUS_OK);
    assert(pop2.type == MK_EVENT_GPIO);
    assert(pop2.param1 == 0x01U);
    assert(mk_event_bus_pending_count() == 0);

    /* 5. Queue overflow handling */
    for (uint32_t i = 0; i < MK_EVENT_QUEUE_SIZE; i++) {
        mk_event_t fill_evt = { .type = MK_EVENT_TIMER, .param1 = i };
        status = mk_event_bus_post(&fill_evt);
        assert(status == MK_STATUS_OK);
    }
    assert(mk_event_bus_pending_count() == MK_EVENT_QUEUE_SIZE);

    /* Posting one more must return QUEUE_FULL */
    mk_event_t overflow_evt = { .type = MK_EVENT_FAULT, .param1 = 999U };
    status = mk_event_bus_post(&overflow_evt);
    assert(status == MK_STATUS_QUEUE_FULL);

    mk_event_bus_stats_t stats;
    mk_event_bus_get_stats(&stats);
    assert(stats.queue_overflows == 1);
    assert(stats.current_depth == MK_EVENT_QUEUE_SIZE);

    /* Clean up queue */
    while (mk_event_bus_pop(&evt) == MK_STATUS_OK) {
        /* drain */
    }
    assert(mk_event_bus_pending_count() == 0);

    printf("[PASS] Kernel Event Bus Unit Tests Passed Successfully.\n");
}
