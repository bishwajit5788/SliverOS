/**
 * @file hal_gpio.c
 * @brief GPIO driver implementation with software debouncing.
 */

#include "hal_gpio.h"
#include "hal_timer.h"
#include <string.h>

#if defined(ESP_PLATFORM)
#include "driver/gpio.h"
#endif

#define MAX_DEBOUNCE_PINS 32U

typedef struct {
    uint8_t last_stable_level;
    uint8_t last_raw_level;
    uint64_t last_change_time_us;
} debounce_state_t;

static debounce_state_t s_debounce_tbl[MAX_DEBOUNCE_PINS];

mk_status_t hal_gpio_init(void)
{
    memset(s_debounce_tbl, 0, sizeof(s_debounce_tbl));
    for (uint32_t i = 0; i < MAX_DEBOUNCE_PINS; i++) {
        s_debounce_tbl[i].last_stable_level = 1U; /* Pullups are active low */
        s_debounce_tbl[i].last_raw_level = 1U;
    }

    (void)hal_gpio_config(HAL_PIN_LED_STATUS, HAL_GPIO_MODE_OUTPUT);
    (void)hal_gpio_config(HAL_PIN_BUTTON_UP, HAL_GPIO_MODE_INPUT_PULLUP);
    (void)hal_gpio_config(HAL_PIN_BUTTON_DOWN, HAL_GPIO_MODE_INPUT_PULLUP);
    (void)hal_gpio_config(HAL_PIN_BUTTON_LEFT, HAL_GPIO_MODE_INPUT_PULLUP);
    (void)hal_gpio_config(HAL_PIN_BUTTON_RIGHT, HAL_GPIO_MODE_INPUT_PULLUP);
    (void)hal_gpio_config(HAL_PIN_BUTTON_A, HAL_GPIO_MODE_INPUT_PULLUP);
    (void)hal_gpio_config(HAL_PIN_BUTTON_B, HAL_GPIO_MODE_INPUT_PULLUP);

    return MK_STATUS_OK;
}

mk_status_t hal_gpio_config(uint32_t pin, hal_gpio_mode_t mode)
{
#if defined(ESP_PLATFORM)
    gpio_config_t io_conf = {0};
    io_conf.pin_bit_mask = (1ULL << pin);

    switch (mode) {
        case HAL_GPIO_MODE_INPUT:
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case HAL_GPIO_MODE_OUTPUT:
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case HAL_GPIO_MODE_INPUT_PULLUP:
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case HAL_GPIO_MODE_INPUT_PULLDOWN:
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        default:
            return MK_STATUS_INVALID_ARG;
    }

    if (gpio_config(&io_conf) != ESP_OK) {
        return MK_STATUS_ERROR;
    }
#else
    (void)pin;
    (void)mode;
#endif
    return MK_STATUS_OK;
}

mk_status_t hal_gpio_write(uint32_t pin, uint8_t level)
{
#if defined(ESP_PLATFORM)
    if (gpio_set_level((gpio_num_t)pin, level != 0 ? 1 : 0) != ESP_OK) {
        return MK_STATUS_ERROR;
    }
#else
    (void)pin;
    (void)level;
#endif
    return MK_STATUS_OK;
}

uint8_t hal_gpio_read(uint32_t pin)
{
#if defined(ESP_PLATFORM)
    return (uint8_t)gpio_get_level((gpio_num_t)pin);
#else
    (void)pin;
    return 1U; /* High by default for active-low buttons */
#endif
}

uint8_t hal_gpio_read_debounced(uint32_t pin, uint32_t debounce_ms)
{
    if (pin >= MAX_DEBOUNCE_PINS) {
        return hal_gpio_read(pin);
    }

    uint8_t raw = hal_gpio_read(pin);
    uint64_t now_us = hal_timer_get_us();
    debounce_state_t *st = &s_debounce_tbl[pin];

    if (raw != st->last_raw_level) {
        st->last_raw_level = raw;
        st->last_change_time_us = now_us;
    }

    if ((now_us - st->last_change_time_us) >= ((uint64_t)debounce_ms * 1000ULL)) {
        st->last_stable_level = raw;
    }

    return st->last_stable_level;
}
