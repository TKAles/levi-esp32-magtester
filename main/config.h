#pragma once

#include <stdint.h>

/* =========================================================================
 * Board / TFT pins (Adafruit ESP32-S3 Reverse TFT Feather)
 * ========================================================================= */
#define TFT_CS_GPIO      7
#define TFT_DC_GPIO      39
#define TFT_RST_GPIO     40
#define TFT_BL_GPIO      45

/* SPI2 bus */
#define SPI_SCK_GPIO     36
#define SPI_MOSI_GPIO    35
#define SPI_MISO_GPIO    37

/* TFT display geometry */
#define TFT_WIDTH        240
#define TFT_HEIGHT       135
#define TFT_GAP_X        40
#define TFT_GAP_Y        53

/* =========================================================================
 * 74HC594 shift register pins
 * ========================================================================= */
#define SR_RCLK_GPIO     12   /* Latch (storage register clock) */
#define SR_SRCLR_GPIO    13   /* Shift register clear, active-low */
#define SR_OE_GPIO       10   /* Output enable, active-low */

/* =========================================================================
 * Buttons
 * ========================================================================= */
#define BUTTON_D0_GPIO   0    /* Mode cycle */
#define BUTTON_D1_GPIO   1    /* Action */
#define BUTTON_DEBOUNCE_MS  50

/* =========================================================================
 * ADC / sensor counts
 * ========================================================================= */
#define NUM_ADCS         7
#define CHANNELS_PER_ADC 4
#define NUM_SENSORS      28   /* 7 * 4 */

/* SPI clock speeds */
#define ADC_SPI_CLOCK_HZ (10 * 1000 * 1000)  /* 10 MHz */
#define TFT_SPI_CLOCK_HZ (40 * 1000 * 1000)  /* 40 MHz */

/* =========================================================================
 * ADC center / reference
 * ========================================================================= */
#define ADC_CENTER       128  /* 1.65 V on 3.3 V supply → 128/255 */

/* =========================================================================
 * NVS
 * ========================================================================= */
#define NVS_NAMESPACE    "magtester"
#define NVS_KEY_LEARNED  "learned"
#define NVS_KEY_HYST     "hyst"
#define NVS_KEY_NSAMPLES "nsamples"
#define NVS_KEY_HAS_DATA "has_data"

/* =========================================================================
 * Default application settings
 * ========================================================================= */
#define DEFAULT_HYSTERESIS  5
#define DEFAULT_NUM_SAMPLES 10
#define MIN_HYSTERESIS      0
#define MAX_HYSTERESIS      50
#define MIN_NUM_SAMPLES     1
#define MAX_NUM_SAMPLES     50

/* =========================================================================
 * Display update interval
 * ========================================================================= */
#define TEST_UPDATE_MS   200
