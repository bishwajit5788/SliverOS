/**
 * @file event_bus.c
 * @brief Kernel event bus implementation with fixed bounded ring buffer.
 */

#include "event_bus.h"
#include "fault_manager.h"
#include <string.h>

static mk_event_t s_queue[MK_EVENT_QUEUE_SIZE];
static uint32_t s_head = 0U;
static uint32_t s_tail = 0U;
static uint32_t s_count = 0U;
static mk_event_bus_stats_t s_stats;
static bool s_initialized = false;

mk_status_t mk_event_bus_init(void)
{
    memset(s_queue, 0, sizeof(s_queue));
    s_head = 0U;
    s_tail = 0U;
    s_count = 0U;
    memset(&s_stats, 0, sizeof(s_stats));
    s_initialized = true;
    return MK_STATUS_OK;
}

mk_status_t mk_event_bus_post(const mk_event_t *event)
{
    if (event == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    if (!s_initialized) {
        if (mk_event_bus_init() != MK_STATUS_OK) {
            return MK_STATUS_ERROR;
        }
    }

    if (s_count >= MK_EVENT_QUEUE_SIZE) {
        s_stats.queue_overflows++;
        mk_fault_record_full(MK_FAULT_APP_ERROR, MK_FAULT_SRC_SYSTEM, MK_FAULT_SEV_WARNING, 0xEE000001, (uint32_t)event->type);
        return MK_STATUS_QUEUE_FULL;
    }

    s_queue[s_head] = *event;
    s_head = (s_head + 1U) % MK_EVENT_QUEUE_SIZE;
    s_count++;

    s_stats.total_events_posted++;
    s_stats.current_depth = s_count;

    return MK_STATUS_OK;
}

mk_status_t mk_event_bus_pop(mk_event_t *out_event)
{
    if (out_event == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    if (!s_initialized || s_count == 0U) {
        return MK_STATUS_QUEUE_EMPTY;
    }

    *out_event = s_queue[s_tail];
    s_tail = (s_tail + 1U) % MK_EVENT_QUEUE_SIZE;
    s_count--;

    s_stats.total_events_dispatched++;
    s_stats.current_depth = s_count;

    return MK_STATUS_OK;
}

mk_status_t mk_event_bus_peek(mk_event_t *out_event)
{
    if (out_event == NULL) {
        return MK_STATUS_INVALID_ARG;
    }

    if (!s_initialized || s_count == 0U) {
        return MK_STATUS_QUEUE_EMPTY;
    }

    *out_event = s_queue[s_tail];
    return MK_STATUS_OK;
}

uint32_t mk_event_bus_pending_count(void)
{
    return s_count;
}

void mk_event_bus_get_stats(mk_event_bus_stats_t *out_stats)
{
    if (out_stats != NULL) {
        *out_stats = s_stats;
        out_stats->current_depth = s_count;
    }
}
