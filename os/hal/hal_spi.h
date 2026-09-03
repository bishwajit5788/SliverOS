/**
 * @file hal_spi.h
 * @brief Master SPI bus driver HAL for display controller communication.
 */

#ifndef MK_HAL_SPI_H
#define MK_HAL_SPI_H

#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_SPI_PIN_MOSI    23U
#define HAL_SPI_PIN_SCLK    18U
#define HAL_SPI_PIN_CS      5U
#define HAL_SPI_PIN_DC      19U
#define HAL_SPI_PIN_RST     21U

mk_status_t hal_spi_init(void);
mk_status_t hal_spi_transmit(const uint8_t *data, size_t length);
mk_status_t hal_spi_write_cmd(uint8_t cmd);
mk_status_t hal_spi_write_data(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* MK_HAL_SPI_H */
