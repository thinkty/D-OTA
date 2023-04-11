
#include "app_ota.h"

static esp_ota_firm_t ota_firm;

esp_err_t init_ota()
{
    /* Initialize ota_firm struct */
    ota_firm.ota_size = 0;
    ota_firm.updating = esp_ota_get_next_update_partition(NULL);
    if (ota_firm.updating == NULL) {
        ESP_LOGE(TAG, "Error while init_ota, unable to find OTA update partition");
        return ESP_FAIL;
    }

    /* Check which OTA partition is running */
    const esp_partition_t * configured = esp_ota_get_boot_partition();
    const esp_partition_t * running = esp_ota_get_running_partition();
    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
    }

    /* Prepare the update partition to write new image */
    esp_err_t err = esp_ota_begin(ota_firm.updating, OTA_SIZE_UNKNOWN, &ota_firm.handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error while init_ota, esp_ota_begin failed error=%d", err);
        return ESP_FAIL;
    }

    /* Connect to the pub/sub bridge server */
    ota_firm.sock = subscribe(OTA_TOPIC);
    if (ota_firm.sock < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t write_ota(char * buf, size_t len)
{
    esp_err_t err = esp_ota_write(ota_firm.handle, (const void *)buf, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error while write_ota, esp_ota_write failed err=0x%x", err);
        return ESP_FAIL;
    }

    ota_firm.ota_size += len;

    return ESP_OK;
}

esp_err_t end_ota()
{
    if (esp_ota_end(ota_firm.handle) != ESP_OK) {
        ESP_LOGE(TAG, "Error while end_ota, esp_ota_end failed!");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_set_boot_partition(ota_firm.updating);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error while end_ota, failed to set boot partition err=0x%x", err);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void task_ota()
{
    if (handle_heartbeat(ota_firm.sock) != ESP_OK) {
        close(ota_firm.sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Receiving new update...");

    if (handle_publish_message(ota_firm.sock, write_ota) != ESP_OK) {
        close(ota_firm.sock);
        vTaskDelete(NULL);
        return;
    }

    close(ota_firm.sock);
    ESP_LOGI(TAG, "Applying patch...");

    if (end_ota() != ESP_OK) {
        close(ota_firm.sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Patch applied, restarting in 5 seconds...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_restart();
}
