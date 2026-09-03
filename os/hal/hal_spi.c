/**
 * @file hal_spi.c
 * @brief Cooperative queued SPI master implementation.
 */
#include "hal_spi.h"
#include "hal_gpio.h"
#include <string.h>
#if defined(ESP_PLATFORM)
#include "driver/spi_master.h"
#define HAL_SPI_QUEUE_DEPTH 8U
typedef struct {
    spi_transaction_t transaction;
    uint8_t dc_level;
    uint8_t command_byte;
    bool in_use;
} hal_spi_slot_t;
static spi_device_handle_t s_spi_handle = NULL;
static hal_spi_slot_t s_slots[HAL_SPI_QUEUE_DEPTH];
static void spi_pre_cb(spi_transaction_t *t)
{
    if (t != NULL && t->user != NULL) {
        const uint8_t *dc = (const uint8_t *)t->user;
        (void)hal_gpio_write(HAL_SPI_PIN_DC, *dc);
    }
}
static hal_spi_slot_t *find_free_slot(void)
{
    for (uint32_t i = 0U; i < HAL_SPI_QUEUE_DEPTH; ++i) if (!s_slots[i].in_use) return &s_slots[i];
    return NULL;
}
static void release_slot(spi_transaction_t *done)
{
    if (done == NULL) return;
    for (uint32_t i = 0U; i < HAL_SPI_QUEUE_DEPTH; ++i) {
        if (&s_slots[i].transaction == done) { s_slots[i].in_use = false; return; }
    }
}
static mk_status_t queue_transfer(const uint8_t *data, size_t length, uint8_t dc)
{
    if (s_spi_handle == NULL) return MK_STATUS_NOT_FOUND;
    hal_spi_slot_t *slot = find_free_slot();
    if (slot == NULL) return MK_STATUS_BUSY;
    memset(&slot->transaction, 0, sizeof(slot->transaction));
    slot->dc_level = dc;
    slot->transaction.length = length * 8U;
    slot->transaction.tx_buffer = data;
    slot->transaction.user = &slot->dc_level;
    slot->transaction.pre_cb = spi_pre_cb;
    slot->in_use = true;
    if (spi_device_queue_trans(s_spi_handle, &slot->transaction, 0) != ESP_OK) {
        slot->in_use = false;
        return MK_STATUS_BUSY;
    }
    return MK_STATUS_OK;
}
#endif

mk_status_t hal_spi_init(void)
{
    (void)hal_gpio_config(HAL_SPI_PIN_DC, HAL_GPIO_MODE_OUTPUT);
    (void)hal_gpio_config(HAL_SPI_PIN_CS, HAL_GPIO_MODE_OUTPUT);
    (void)hal_gpio_config(HAL_SPI_PIN_RST, HAL_GPIO_MODE_OUTPUT);
    (void)hal_gpio_write(HAL_SPI_PIN_CS, 1U);
    (void)hal_gpio_write(HAL_SPI_PIN_DC, 1U);
#if defined(ESP_PLATFORM)
    memset(s_slots, 0, sizeof(s_slots));
    spi_bus_config_t buscfg = {
        .miso_io_num = -1, .mosi_io_num = HAL_SPI_PIN_MOSI, .sclk_io_num = HAL_SPI_PIN_SCLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 4096
    };
    if (spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) return MK_STATUS_ERROR;
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000, .mode = 0, .spics_io_num = HAL_SPI_PIN_CS,
        .queue_size = HAL_SPI_QUEUE_DEPTH, .pre_cb = NULL
    };
    if (spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle) != ESP_OK) return MK_STATUS_ERROR;
#endif
    return MK_STATUS_OK;
}

void hal_spi_service(void)
{
#if defined(ESP_PLATFORM)
    if (s_spi_handle == NULL) return;
    spi_transaction_t *done = NULL;
    while (spi_device_get_trans_result(s_spi_handle, &done, 0) == ESP_OK) release_slot(done);
#endif
}

mk_status_t hal_spi_transmit(const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0U) return MK_STATUS_INVALID_ARG;
#if defined(ESP_PLATFORM)
    hal_spi_service();
    return queue_transfer(data, length, 1U);
#else
    return MK_STATUS_OK;
#endif
}

mk_status_t hal_spi_write_cmd(uint8_t cmd)
{
#if defined(ESP_PLATFORM)
    hal_spi_service();
    hal_spi_slot_t *slot = find_free_slot();
    if (slot == NULL) return MK_STATUS_BUSY;
    slot->command_byte = cmd;
    memset(&slot->transaction, 0, sizeof(slot->transaction));
    slot->dc_level = 0U;
    slot->transaction.length = 8U;
    slot->transaction.tx_buffer = &slot->command_byte;
    slot->transaction.user = &slot->dc_level;
    slot->transaction.pre_cb = spi_pre_cb;
    slot->in_use = true;
    if (spi_device_queue_trans(s_spi_handle, &slot->transaction, 0) != ESP_OK) { slot->in_use = false; return MK_STATUS_BUSY; }
    return MK_STATUS_OK;
#else
    (void)cmd; return MK_STATUS_OK;
#endif
}

mk_status_t hal_spi_write_data(const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0U) return MK_STATUS_INVALID_ARG;
#if defined(ESP_PLATFORM)
    hal_spi_service();
    return queue_transfer(data, length, 1U);
#else
    return MK_STATUS_OK;
#endif
}
