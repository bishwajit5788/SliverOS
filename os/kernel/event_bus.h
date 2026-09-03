/**
 * @file event_bus.h
 * @brief Kernel event bus using a fixed-size bounded ring buffer.
 * Provides decoupled asynchronous signaling between ISRs/drivers and cooperative tasks.
 */

#ifndef MK_EVENT_BUS_H
#define MK_EVENT_BUS_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t total_events_posted;
    uint32_t total_events_dispatched;
    uint32_t queue_overflows;
    uint32_t current_depth;
} mk_event_bus_stats_t;

/**
 * @brief Initialize the static event bus ring buffer.
 */
mk_status_t mk_event_bus_init(void);

/**
 * @brief Post an event into the bounded ring buffer.
 * Safe to call from ISR or driver callback context (minimal non-blocking bounded copy).
 *
 * @param event Pointer to event structure to copy.
 * @return MK_STATUS_OK if enqueued, MK_STATUS_QUEUE_FULL if dropped.
 */
mk_status_t mk_event_bus_post(const mk_event_t *event);

/**
 * @brief Pop the oldest event from the queue.
 *
 * @param out_event Destination event buffer.
 * @return MK_STATUS_OK if retrieved, MK_STATUS_QUEUE_EMPTY if no events pending.
 */
mk_status_t mk_event_bus_pop(mk_event_t *out_event);

/**
 * @brief Peek at the oldest event without removing it.
 */
mk_status_t mk_event_bus_peek(mk_event_t *out_event);

/**
 * @brief Current pending event count.
 */
uint32_t mk_event_bus_pending_count(void);

/**
 * @brief Retrieve event bus statistics.
 */
void mk_event_bus_get_stats(mk_event_bus_stats_t *out_stats);

#ifdef __cplusplus
}
#endif

#endif /* MK_EVENT_BUS_H */
