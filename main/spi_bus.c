#include "spi_bus.h"
#include "config.h"

#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "spi_bus";

static spi_device_handle_t s_adc_handle = NULL;

esp_err_t spi_bus_init(void)
{
    /* ------------------------------------------------------------------
     * Configure SPI2 bus
     * ------------------------------------------------------------------ */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = SPI_MOSI_GPIO,
        .miso_io_num     = SPI_MISO_GPIO,
        .sclk_io_num     = SPI_SCK_GPIO,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        /* Large enough for a full TFT frame plus overhead */
        .max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * 2 + 8,
        .flags           = 0,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ------------------------------------------------------------------
     * Add the ADC / shift-register device.
     * Software CS (cs_io_num = -1): the 74HC594 manages chip selects.
     * SPI Mode 0 (CPOL=0, CPHA=0) as required by ADC084S021.
     * ------------------------------------------------------------------ */
    spi_device_interface_config_t dev_cfg = {
        .command_bits     = 0,
        .address_bits     = 0,
        .dummy_bits       = 0,
        .mode             = 0,          /* SPI mode 0 */
        .duty_cycle_pos   = 128,
        .cs_ena_pretrans  = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz   = ADC_SPI_CLOCK_HZ,
        .input_delay_ns   = 0,
        .spics_io_num     = -1,         /* Software CS */
        .flags            = 0,
        .queue_size       = 1,
        .pre_cb           = NULL,
        .post_cb          = NULL,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device (ADC) failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI2 bus + ADC device initialised");
    return ESP_OK;
}

spi_device_handle_t spi_get_adc_handle(void)
{
    return s_adc_handle;
}
