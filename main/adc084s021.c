#include "adc084s021.h"
#include "hc594.h"
#include "spi_bus.h"
#include "config.h"

#include "driver/spi_master.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "adc084s021";

/*
 * ADC084S021 channel address encoding in the TX word:
 *   Bit 13 = ADD1, Bit 12 = ADD0 (within 16-bit TX frame, MSB first)
 *
 *   CH0 → 0x0000
 *   CH1 → 0x0800  (ADD1=0, ADD0=1  → bits [12]=1 in a 16-bit word)
 *   CH2 → 0x1000  (ADD1=1, ADD0=0)
 *   CH3 → 0x1800  (ADD1=1, ADD0=1)
 */
static const uint16_t CHANNEL_CMD[4] = {
    0x0000,   /* CH0 */
    0x0800,   /* CH1 */
    0x1000,   /* CH2 */
    0x1800,   /* CH3 */
};

uint8_t adc084s021_read_channel(uint8_t adc_num, uint8_t channel)
{
    if (adc_num < 1 || adc_num > NUM_ADCS) {
        ESP_LOGE(TAG, "Invalid adc_num %u", adc_num);
        return 0;
    }
    if (channel > 3) {
        ESP_LOGE(TAG, "Invalid channel %u", channel);
        return 0;
    }

    spi_device_handle_t spi = spi_get_adc_handle();

    uint16_t tx_data = CHANNEL_CMD[channel];
    uint16_t rx_data = 0;

    /* Convert to big-endian for SPI transmission */
    uint8_t tx_buf[2] = { (uint8_t)(tx_data >> 8), (uint8_t)(tx_data & 0xFF) };
    uint8_t rx_buf[2] = { 0, 0 };

    spi_transaction_t t = {
        .length    = 16,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
        .flags     = 0,
    };

    /* Select the target ADC via the shift register */
    hc594_select(adc_num);

    spi_device_polling_transmit(spi, &t);

    /* Deselect (disable /OE) */
    hc594_deselect();

    /* Result is in bits [11:4] of the 16-bit reply */
    rx_data = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
    return (uint8_t)((rx_data >> 4) & 0xFF);
}
