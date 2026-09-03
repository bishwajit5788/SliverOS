/**
 * @file input.c
 * @brief Debounced game input controller implementation.
 */

#include "input.h"
#include "hal_gpio.h"

#define INPUT_DEBOUNCE_MS 15U

mk_status_t input_init(void)
{
    (void)hal_gpio_init();
    return MK_STATUS_OK;
}

uint8_t input_get_state(void)
{
    uint8_t state = 0U;

    /* Active low button inputs */
    if (hal_gpio_read_debounced(HAL_PIN_BUTTON_UP, INPUT_DEBOUNCE_MS) == 0U) {
        state |= INPUT_BTN_UP;
    }
    if (hal_gpio_read_debounced(HAL_PIN_BUTTON_DOWN, INPUT_DEBOUNCE_MS) == 0U) {
        state |= INPUT_BTN_DOWN;
    }
    if (hal_gpio_read_debounced(HAL_PIN_BUTTON_LEFT, INPUT_DEBOUNCE_MS) == 0U) {
        state |= INPUT_BTN_LEFT;
    }
    if (hal_gpio_read_debounced(HAL_PIN_BUTTON_RIGHT, INPUT_DEBOUNCE_MS) == 0U) {
        state |= INPUT_BTN_RIGHT;
    }
    if (hal_gpio_read_debounced(HAL_PIN_BUTTON_A, INPUT_DEBOUNCE_MS) == 0U) {
        state |= INPUT_BTN_A;
    }
    if (hal_gpio_read_debounced(HAL_PIN_BUTTON_B, INPUT_DEBOUNCE_MS) == 0U) {
        state |= INPUT_BTN_B;
    }

    return state;
}
