/**
 * @file hal_timer.h
 * @brief Hardware timer and monotonic timebase HAL.
 */

#ifndef MK_HAL_TIMER_H
#define MK_HAL_TIMER_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

mk_status_t hal_timer_init(void);
uint64_t hal_timer_get_us(void);
uint32_t hal_timer_get_ms(void);
void hal_timer_delay_ms(uint32_t ms);
void hal_timer_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* MK_HAL_TIMER_H */
