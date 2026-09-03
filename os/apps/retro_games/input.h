/**
 * @file input.h
 * @brief Debounced game input controller abstraction.
 */

#ifndef MK_INPUT_H
#define MK_INPUT_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INPUT_BTN_UP      0x01U
#define INPUT_BTN_DOWN    0x02U
#define INPUT_BTN_LEFT    0x04U
#define INPUT_BTN_RIGHT   0x08U
#define INPUT_BTN_A       0x10U
#define INPUT_BTN_B       0x20U

mk_status_t input_init(void);
uint8_t input_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* MK_INPUT_H */
