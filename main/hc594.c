#include "hc594.h"
#include "spi_bus.h"
#include "config.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include <string.h>

static const char *TAG = "hc594";

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

static inline void rclk_pulse(void)
{
    gpio_set_level(SR_RCLK_GPIO, 1);
    ets_delay_us(1);
    gpio_set_level(SR_RCLK_GPIO, 0);
    ets_delay_us(1);
}

/**
 * @brief Shift one byte into the SR using the shared SPI device.
 *
 * The 74HC594 samples on the rising edge of SRCLK (SPI CLK), which
 * matches SPI mode 0.  We send 8 bits MSB-first.
 */
static void sr_write_byte(uint8_t data)
{
    spi_device_handle_t spi = spi_get_adc_handle();

    spi_transaction_t t = {
        .length    = 8,              /* bits */
        .tx_buffer = &data,
        .rx_buffer = NULL,
        .flags     = 0,
    };

    spi_device_polling_transmit(spi, &t);
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

esp_err_t hc594_init(void)
{
    /* Configure RCLK, /SRCLR, /OE as outputs */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SR_RCLK_GPIO)  |
                        (1ULL << SR_SRCLR_GPIO)  |
                        (1ULL << SR_OE_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Initial state: outputs disabled, RCLK low */
    gpio_set_level(SR_OE_GPIO,    1);   /* /OE high → outputs disabled */
    gpio_set_level(SR_RCLK_GPIO,  0);
    gpio_set_level(SR_SRCLR_GPIO, 1);   /* keep clear de-asserted */

    /* Pulse /SRCLR low → high to clear the shift register */
    gpio_set_level(SR_SRCLR_GPIO, 0);
    ets_delay_us(2);
    gpio_set_level(SR_SRCLR_GPIO, 1);
    ets_delay_us(2);

    /* Pre-load 0xFF (all CS deasserted) into storage register */
    sr_write_byte(0xFF);
    rclk_pulse();

    ESP_LOGI(TAG, "74HC594 initialised");
    return ESP_OK;
}

void hc594_select(uint8_t adc_num)
{
    /* cs_byte: bit (N-1) is 0 (active-low), all others 1 */
    uint8_t cs_byte = 0xFF & (uint8_t)(~(1u << (adc_num - 1)));

    sr_write_byte(cs_byte);
    rclk_pulse();

    /* Enable outputs */
    gpio_set_level(SR_OE_GPIO, 0);
    ets_delay_us(1);
}

void hc594_deselect(void)
{
    /* Disable outputs only — storage register keeps its value */
    gpio_set_level(SR_OE_GPIO, 1);
    ets_delay_us(1);
}
