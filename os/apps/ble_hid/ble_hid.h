/**
 * @file ble_hid.h
 * @brief BLE HID keyboard application module.
 */

#ifndef MK_APP_BLE_HID_H
#define MK_APP_BLE_HID_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

mk_status_t ble_hid_init(void);
mk_status_t ble_hid_start(void);
mk_status_t ble_hid_stop(void);
void ble_hid_task(void *context);

#ifdef __cplusplus
}
#endif

#endif /* MK_APP_BLE_HID_H */
