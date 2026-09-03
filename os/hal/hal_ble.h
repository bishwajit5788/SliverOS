/**
 * @file hal_ble.h
 * @brief Bluetooth Low Energy HID (Human Interface Device) keyboard HAL.
 */

#ifndef MK_HAL_BLE_H
#define MK_HAL_BLE_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HID Modifier Bitmasks */
#define HAL_BLE_MOD_NONE        0x00U
#define HAL_BLE_MOD_LCTRL       0x01U
#define HAL_BLE_MOD_LSHIFT      0x02U
#define HAL_BLE_MOD_LALT        0x04U
#define HAL_BLE_MOD_LGUI        0x08U
#define HAL_BLE_MOD_RCTRL       0x10U
#define HAL_BLE_MOD_RSHIFT      0x20U
#define HAL_BLE_MOD_RALT        0x40U
#define HAL_BLE_MOD_RGUI        0x80U

/* Standard USB HID Keycodes */
#define HAL_BLE_KEY_NONE        0x00U
#define HAL_BLE_KEY_A           0x04U
#define HAL_BLE_KEY_B           0x05U
#define HAL_BLE_KEY_C           0x06U
#define HAL_BLE_KEY_D           0x07U
#define HAL_BLE_KEY_E           0x08U
#define HAL_BLE_KEY_ENTER       0x28U
#define HAL_BLE_KEY_ESCAPE      0x29U
#define HAL_BLE_KEY_BACKSPACE   0x2AU
#define HAL_BLE_KEY_TAB         0x2BU
#define HAL_BLE_KEY_SPACE       0x2CU
#define HAL_BLE_KEY_CAPSLOCK    0x39U

/**
 * @brief Standard 8-byte HID keyboard report structure.
 */
typedef struct {
    uint8_t modifiers;      /* Bitmask of modifiers */
    uint8_t reserved;       /* Constant 0x00 */
    uint8_t keycodes[6];    /* Up to 6 concurrent keypresses */
} hal_ble_hid_report_t;

mk_status_t hal_ble_init(void);
mk_status_t hal_ble_start_advertising(void);
mk_status_t hal_ble_stop_advertising(void);
bool hal_ble_is_connected(void);
mk_status_t hal_ble_send_report(const hal_ble_hid_report_t *report);
mk_status_t hal_ble_send_key(uint8_t modifier, uint8_t keycode);
mk_status_t hal_ble_send_release(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_HAL_BLE_H */
