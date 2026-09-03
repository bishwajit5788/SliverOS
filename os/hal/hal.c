/**
 * @file hal.c
 * @brief Master Hardware Abstraction Layer coordinator.
 */

#include "hal.h"
#include "fault_manager.h"

mk_status_t hal_init(void)
{
    mk_status_t status;

    status = hal_timer_init();
    if (status != MK_STATUS_OK) {
        mk_fault_record_full(MK_FAULT_HAL_FAIL, MK_FAULT_SRC_HAL, MK_FAULT_SEV_CRITICAL, 0x01);
        return status;
    }

    status = hal_gpio_init();
    if (status != MK_STATUS_OK) {
        mk_fault_record_full(MK_FAULT_HAL_FAIL, MK_FAULT_SRC_HAL, MK_FAULT_SEV_CRITICAL, 0x02);
        return status;
    }

    status = hal_spi_init();
    if (status != MK_STATUS_OK) {
        mk_fault_record_full(MK_FAULT_HAL_FAIL, MK_FAULT_SRC_HAL, MK_FAULT_SEV_CRITICAL, 0x03);
        return status;
    }

    status = hal_wifi_init();
    if (status != MK_STATUS_OK) {
        mk_fault_record_full(MK_FAULT_HAL_FAIL, MK_FAULT_SRC_HAL, MK_FAULT_SEV_WARNING, 0x04);
    }

    status = hal_ble_init();
    if (status != MK_STATUS_OK) {
        mk_fault_record_full(MK_FAULT_HAL_FAIL, MK_FAULT_SRC_HAL, MK_FAULT_SEV_WARNING, 0x05);
    }

    return MK_STATUS_OK;
}
