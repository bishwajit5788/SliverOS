/**
 * @file hal.h
 * @brief Master Hardware Abstraction Layer interface.
 */

#ifndef MK_HAL_H
#define MK_HAL_H

#include "kernel_types.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_wifi.h"
#include "hal_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all hardware abstraction subsystems.
 * @return MK_STATUS_OK on success, error code otherwise.
 */
mk_status_t hal_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_HAL_H */
