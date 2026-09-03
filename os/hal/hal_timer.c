/**
 * @file hal_timer.c
 * @brief Hardware timer and monotonic timebase implementation.
 *
 * The scheduler uses a monotonic clock. Host tests use POSIX monotonic time.
 * Delay helpers are HAL-only primitives; cooperative application dispatch must
 * never call them as a blocking scheduling mechanism.
 */

#if !defined(ESP_PLATFORM)
#define _POSIX_C_SOURCE 200809L
#endif

#include "hal_timer.h"

#if defined(ESP_PLATFORM)
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include <time.h>
#endif

mk_status_t hal_timer_init(void)
{
    return MK_STATUS_OK;
}

uint64_t hal_timer_get_us(void)
{
#if defined(ESP_PLATFORM)
    return (uint64_t)esp_timer_get_time();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0ULL;
    }
    return ((uint64_t)ts.tv_sec * 1000000ULL) + ((uint64_t)ts.tv_nsec / 1000ULL);
#endif
}

uint32_t hal_timer_get_ms(void)
{
    return (uint32_t)(hal_timer_get_us() / 1000ULL);
}

void hal_timer_delay_ms(uint32_t ms)
{
#if defined(ESP_PLATFORM)
    if (ms >= portTICK_PERIOD_MS) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        ets_delay_us(ms * 1000U);
    }
#else
    struct timespec req = {
        .tv_sec = (time_t)(ms / 1000U),
        .tv_nsec = (long)(ms % 1000U) * 1000000L
    };
    (void)nanosleep(&req, NULL);
#endif
}

void hal_timer_delay_us(uint32_t us)
{
#if defined(ESP_PLATFORM)
    ets_delay_us(us);
#else
    struct timespec req = {
        .tv_sec = (time_t)(us / 1000000U),
        .tv_nsec = (long)(us % 1000000U) * 1000L
    };
    (void)nanosleep(&req, NULL);
#endif
}
