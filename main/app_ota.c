
#include "app_ota.h"


esp_err_t init_ota(esp_ota_firm_t * ota_firm)
{
    // Initialize ota_firm struct
    ota_firm->ota_size = 0;
    ota_firm->updating = esp_ota_get_next_update_partition(NULL);
    if (ota_firm->updating == NULL) {
        ESP_LOGE(TAG, "Error: init_ota, unable to find OTA update partition");
        return ESP_FAIL;
    }

    // Check which OTA partition is running
    const esp_partition_t * configured = esp_ota_get_boot_partition();
    const esp_partition_t * running = esp_ota_get_running_partition();
    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
    }

    // Prepare the update partition to write new image
    esp_err_t err = esp_ota_begin(ota_firm->updating, OTA_SIZE_UNKNOWN, &ota_firm->handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: init_ota, esp_ota_begin failed error=%d", err);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t write_ota(esp_ota_firm_t * ota_firm, char * buf, size_t len)
{
    esp_err_t err = esp_ota_write(ota_firm->handle, (const void *)buf, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: write_ota, esp_ota_write failed err=0x%x", err);
        return ESP_FAIL;
    }

    ota_firm->ota_size += len;

    return ESP_OK;
}

esp_err_t end_ota(esp_ota_firm_t * ota_firm)
{
    if (esp_ota_end(ota_firm->handle) != ESP_OK) {
        ESP_LOGE(TAG, "Error: end_ota, esp_ota_end failed!");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_set_boot_partition(ota_firm->updating);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: end_ota, failed to set boot partition err=0x%x", err);
        return ESP_FAIL;
    }

    return ESP_OK;
}
