#include "nvs_storage.h"
#include "config.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "nvs_storage";

/* NVS handle; opened once and held for the application lifetime */
static nvs_handle_t s_nvs_handle = 0;

/* =========================================================================
 * Initialisation
 * ========================================================================= */

esp_err_t nvs_storage_init(void)
{
    /* Initialise default NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition corrupt; erasing and reinitialising");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Open namespace */
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "NVS initialised (namespace: %s)", NVS_NAMESPACE);
    return ESP_OK;
}

/* =========================================================================
 * Learned data
 * ========================================================================= */

esp_err_t nvs_storage_save_learned(const uint8_t data[28])
{
    esp_err_t ret;

    /* Store the 28-byte blob */
    ret = nvs_set_blob(s_nvs_handle, NVS_KEY_LEARNED, data, NUM_SENSORS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_blob (%s) failed: %s", NVS_KEY_LEARNED, esp_err_to_name(ret));
        return ret;
    }

    /* Mark that we have valid data */
    ret = nvs_set_u8(s_nvs_handle, NVS_KEY_HAS_DATA, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_u8 (%s) failed: %s", NVS_KEY_HAS_DATA, esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t nvs_storage_load_learned(uint8_t data[28], uint8_t *has_data)
{
    /* Check has_data flag first */
    uint8_t flag = 0;
    nvs_get_u8(s_nvs_handle, NVS_KEY_HAS_DATA, &flag);
    *has_data = flag;

    if (!flag) {
        memset(data, ADC_CENTER, NUM_SENSORS);
        return ESP_OK;
    }

    size_t length = NUM_SENSORS;
    esp_err_t ret = nvs_get_blob(s_nvs_handle, NVS_KEY_LEARNED, data, &length);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_blob (%s) failed: %s", NVS_KEY_LEARNED, esp_err_to_name(ret));
        memset(data, ADC_CENTER, NUM_SENSORS);
        *has_data = 0;
    }
    return ret;
}

/* =========================================================================
 * Hysteresis
 * ========================================================================= */

esp_err_t nvs_storage_save_hyst(uint8_t hyst)
{
    esp_err_t ret = nvs_set_u8(s_nvs_handle, NVS_KEY_HYST, hyst);
    if (ret == ESP_OK) ret = nvs_commit(s_nvs_handle);
    return ret;
}

uint8_t nvs_storage_load_hyst(void)
{
    uint8_t val = DEFAULT_HYSTERESIS;
    nvs_get_u8(s_nvs_handle, NVS_KEY_HYST, &val);
    return val;
}

/* =========================================================================
 * Num samples
 * ========================================================================= */

esp_err_t nvs_storage_save_nsamples(uint8_t nsamples)
{
    esp_err_t ret = nvs_set_u8(s_nvs_handle, NVS_KEY_NSAMPLES, nsamples);
    if (ret == ESP_OK) ret = nvs_commit(s_nvs_handle);
    return ret;
}

uint8_t nvs_storage_load_nsamples(void)
{
    uint8_t val = DEFAULT_NUM_SAMPLES;
    nvs_get_u8(s_nvs_handle, NVS_KEY_NSAMPLES, &val);
    return val;
}
