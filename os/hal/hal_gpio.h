/**
 * @file hal_gpio.h
 * @brief GPIO driver HAL with software debouncing.
 */

#ifndef MK_HAL_GPIO_H
#define MK_HAL_GPIO_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_GPIO_MODE_INPUT = 0,
    HAL_GPIO_MODE_OUTPUT,
    HAL_GPIO_MODE_INPUT_PULLUP,
    HAL_GPIO_MODE_INPUT_PULLDOWN
} hal_gpio_mode_t;

/* Default Game & Navigation Pin Definitions */
#define HAL_PIN_BUTTON_UP       12U
#define HAL_PIN_BUTTON_DOWN     13U
#define HAL_PIN_BUTTON_LEFT     14U
#define HAL_PIN_BUTTON_RIGHT    27U
#define HAL_PIN_BUTTON_A        25U
#define HAL_PIN_BUTTON_B        26U
#define HAL_PIN_LED_STATUS      2U

mk_status_t hal_gpio_init(void);
mk_status_t hal_gpio_config(uint32_t pin, hal_gpio_mode_t mode);
mk_status_t hal_gpio_write(uint32_t pin, uint8_t level);
uint8_t hal_gpio_read(uint32_t pin);
uint8_t hal_gpio_read_debounced(uint32_t pin, uint32_t debounce_ms);

#ifdef __cplusplus
}
#endif

#endif /* MK_HAL_GPIO_H */
