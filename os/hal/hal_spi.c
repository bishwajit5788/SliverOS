/**
 * @file hal_spi.c
 * @brief Master SPI bus driver implementation.
 */

#include "hal_spi.h"
#include "hal_gpio.h"
#include <string.h>

#if defined(ESP_PLATFORM)
#include "driver/spi_master.h"
static spi_device_handle_t s_spi_handle = NULL;
#endif

mk_status_t hal_spi_init(void)
{
    (void)hal_gpio_config(HAL_SPI_PIN_DC, HAL_GPIO_MODE_OUTPUT);
    (void)hal_gpio_config(HAL_SPI_PIN_CS, HAL_GPIO_MODE_OUTPUT);
    (void)hal_gpio_config(HAL_SPI_PIN_RST, HAL_GPIO_MODE_OUTPUT);

    (void)hal_gpio_write(HAL_SPI_PIN_CS, 1U);
    (void)hal_gpio_write(HAL_SPI_PIN_DC, 1U);

#if defined(ESP_PLATFORM)
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = HAL_SPI_PIN_MOSI,
        .sclk_io_num = HAL_SPI_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    if (spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) {
        return MK_STATUS_ERROR;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000, /* 20 MHz */
        .mode = 0,
        .spics_io_num = HAL_SPI_PIN_CS,
        .queue_size = 7
    };

    if (spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle) != ESP_OK) {
        return MK_STATUS_ERROR;
    }
#endif
    return MK_STATUS_OK;
}

mk_status_t hal_spi_transmit(const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0) {
        return MK_STATUS_INVALID_ARG;
    }

#if defined(ESP_PLATFORM)
    if (s_spi_handle == NULL) {
        return MK_STATUS_NOT_FOUND;
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = length * 8U;
    t.tx_buffer = data;

    if (spi_device_polling_transmit(s_spi_handle, &t) != ESP_OK) {
        return MK_STATUS_ERROR;
    }
#endif
    return MK_STATUS_OK;
}

mk_status_t hal_spi_write_cmd(uint8_t cmd)
{
    (void)hal_gpio_write(HAL_SPI_PIN_DC, 0U); /* D/C low = command */
    return hal_spi_transmit(&cmd, 1U);
}

mk_status_t hal_spi_write_data(const uint8_t *data, size_t length)
{
    (void)hal_gpio_write(HAL_SPI_PIN_DC, 1U); /* D/C high = data */
    return hal_spi_transmit(data, length);
}
