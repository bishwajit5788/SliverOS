/**
 * @file event_bus.c
 * @brief Fixed bounded event bus. Overflow policy is DROP_NEWEST for kernel
 * events; producers never block. ISR/callback posting uses a short critical
 * section and never performs fault logging while interrupts are masked.
 */
#include "event_bus.h"
#include "fault_manager.h"
#include <string.h>
#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
static portMUX_TYPE s_queue_lock = portMUX_INITIALIZER_UNLOCKED;
#endif

static mk_event_t s_queue[MK_EVENT_QUEUE_SIZE];
static uint32_t s_head = 0U, s_tail = 0U, s_count = 0U;
static mk_event_bus_stats_t s_stats;
static bool s_initialized = false;

mk_status_t mk_event_bus_init(void)
{
#if defined(ESP_PLATFORM)
    portENTER_CRITICAL(&s_queue_lock);
#endif
    memset(s_queue, 0, sizeof(s_queue));
    s_head = s_tail = s_count = 0U;
    memset(&s_stats, 0, sizeof(s_stats));
    s_initialized = true;
#if defined(ESP_PLATFORM)
    portEXIT_CRITICAL(&s_queue_lock);
#endif
    return MK_STATUS_OK;
}

mk_status_t mk_event_bus_post(const mk_event_t *event)
{
    if (event == NULL) return MK_STATUS_INVALID_ARG;
    if (!s_initialized && mk_event_bus_init() != MK_STATUS_OK) return MK_STATUS_ERROR;

    bool overflow = false;
#if defined(ESP_PLATFORM)
    portENTER_CRITICAL_ISR(&s_queue_lock);
#endif
    if (s_count >= MK_EVENT_QUEUE_SIZE) {
        ++s_stats.queue_overflows;
        overflow = true; /* DROP_NEWEST: preserve already queued events. */
    } else {
        s_queue[s_head] = *event;
        s_head = (s_head + 1U) % MK_EVENT_QUEUE_SIZE;
        ++s_count;
        ++s_stats.total_events_posted;
        s_stats.current_depth = s_count;
    }
#if defined(ESP_PLATFORM)
    portEXIT_CRITICAL_ISR(&s_queue_lock);
#endif

    if (overflow) {
        mk_fault_record_full(MK_FAULT_APP_ERROR, MK_FAULT_SRC_SYSTEM,
                             MK_FAULT_SEV_WARNING, 0xEE000001U, (uint32_t)event->type);
        return MK_STATUS_QUEUE_FULL;
    }
    return MK_STATUS_OK;
}

mk_status_t mk_event_bus_pop(mk_event_t *out_event)
{
    if (out_event == NULL) return MK_STATUS_INVALID_ARG;
    mk_status_t status = MK_STATUS_QUEUE_EMPTY;
#if defined(ESP_PLATFORM)
    portENTER_CRITICAL(&s_queue_lock);
#endif
    if (s_initialized && s_count > 0U) {
        *out_event = s_queue[s_tail];
        s_tail = (s_tail + 1U) % MK_EVENT_QUEUE_SIZE;
        --s_count;
        ++s_stats.total_events_dispatched;
        s_stats.current_depth = s_count;
        status = MK_STATUS_OK;
    }
#if defined(ESP_PLATFORM)
    portEXIT_CRITICAL(&s_queue_lock);
#endif
    return status;
}

mk_status_t mk_event_bus_peek(mk_event_t *out_event)
{
    if (out_event == NULL) return MK_STATUS_INVALID_ARG;
#if defined(ESP_PLATFORM)
    portENTER_CRITICAL(&s_queue_lock);
#endif
    if (!s_initialized || s_count == 0U) {
#if defined(ESP_PLATFORM)
        portEXIT_CRITICAL(&s_queue_lock);
#endif
        return MK_STATUS_QUEUE_EMPTY;
    }
    *out_event = s_queue[s_tail];
#if defined(ESP_PLATFORM)
    portEXIT_CRITICAL(&s_queue_lock);
#endif
    return MK_STATUS_OK;
}

uint32_t mk_event_bus_pending_count(void)
{
    uint32_t count;
#if defined(ESP_PLATFORM)
    portENTER_CRITICAL(&s_queue_lock);
#endif
    count = s_count;
#if defined(ESP_PLATFORM)
    portEXIT_CRITICAL(&s_queue_lock);
#endif
    return count;
}

void mk_event_bus_get_stats(mk_event_bus_stats_t *out_stats)
{
    if (out_stats == NULL) return;
#if defined(ESP_PLATFORM)
    portENTER_CRITICAL(&s_queue_lock);
#endif
    *out_stats = s_stats;
    out_stats->current_depth = s_count;
#if defined(ESP_PLATFORM)
    portEXIT_CRITICAL(&s_queue_lock);
#endif
}
