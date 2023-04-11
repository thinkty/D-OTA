
#include "app_dota.h"

static esp_dota_firm_t dota_firm;

esp_err_t init_dota()
{
    /* Initialize ota_firm struct */
    dota_firm.ota_size = 0;
    dota_firm.updating = esp_ota_get_next_update_partition(NULL);
    if (dota_firm.updating == NULL) {
        ESP_LOGE(TAG, "Error while init_ota, unable to find OTA update partition");
        return ESP_FAIL;
    }

    /* Check which OTA partition is running */
    const esp_partition_t * configured = esp_ota_get_boot_partition();
    dota_firm.running = esp_ota_get_running_partition();
    dota_firm.running_offset = 0;
    if (configured != dota_firm.running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, dota_firm.running->address);
    }

    /* Prepare the update partition to write new image */
    esp_err_t err = esp_ota_begin(dota_firm.updating, OTA_SIZE_UNKNOWN, &dota_firm.handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error while init_ota, esp_ota_begin failed error=%d", err);
        return ESP_FAIL;
    }

    /* Initialize and mount SPIFFS filesystem */
    esp_vfs_spiffs_conf_t conf = {
      .base_path = DOTA_SPIFF,
      .partition_label = NULL,
      .max_files = 1,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    /* Open delta file */
    dota_firm.delta = fopen(DOTA_SPIFF DOTA_DELTA, "wb");
    if (dota_firm.delta == NULL) {
        ESP_LOGE(TAG, "Failed to open delta file");
        return ESP_FAIL;
    }
    dota_firm.delta_size = 0;

    /* Connect to the pub/sub bridge server */
    dota_firm.sock = subscribe(DOTA_TOPIC);
    if (dota_firm.sock < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t write_delta(char * buf, size_t len)
{
    if (fwrite(buf, 1, len, dota_firm.delta) != len) {
        ESP_LOGE(TAG, "Failed to write to delta file");
        return ESP_FAIL;
    }

    dota_firm.delta_size += len;
    return ESP_OK;
}

/**
 * @brief Callback to read from the currently running partition's image.
 * 
 * @see detools_apply_patch_callbacks()
 * 
 * @param arg_p Pointing to the dota_firm_t passed in detools_apply_patch_callbacks()
 * @param buf_p Buffer to store the read bytes from the current running image
 * @param size Number of bytes to read
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
static int read_current_image(void * arg_p, uint8_t * buf_p, size_t size)
{
    esp_dota_firm_t * firm = (esp_dota_firm_t *) arg_p;

    if (!firm) {
        ESP_LOGE(TAG, "Error: read_current_image - invalid argument");
        return ESP_FAIL;
    }
    if (size <= 0) {
        ESP_LOGE(TAG, "Error: read_current_image - invalid size requested");
        return ESP_FAIL;
    }

    /* Read from the currently running image's partition */
    if (esp_partition_read(firm->running, firm->running_offset, buf_p, size) != ESP_OK) {
        ESP_LOGE(TAG, "Error: read_current_image - esp_partition_read");
        return ESP_FAIL;
    }
    firm->running_offset += size;

    /* If it goes over the set partition size, error */
    if (firm->running_offset >= firm->running->size) {
        ESP_LOGE(TAG, "Error: read_current_image - offset is too big");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Callback to move offset from the currently running partition's image.
 * 
 * @see detools_apply_patch_callbacks()
 * 
 * @param arg_p Pointing to the dota_firm_t passed in detools_apply_patch_callbacks()
 * @param offset Amount of offset to move
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
static int seek_current_image(void * arg_p, int offset)
{
    esp_dota_firm_t * firm = (esp_dota_firm_t *) arg_p;

    if (!firm) {
        ESP_LOGE(TAG, "Error: seek_current_image - invalid argument");
        return ESP_FAIL;
    }

    firm->running_offset += offset;

    /* If it goes over the set partition size, error */
    if (firm->running_offset >= firm->running->size) {
        ESP_LOGE(TAG, "Error: seek_current_image - offset is too big");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Callback to read from the delta file.
 * 
 * @see detools_apply_patch_callbacks()
 * 
 * @param arg_p Pointing to the dota_firm_t passed in detools_apply_patch_callbacks()
 * @param buf_p Buffer to store the read bytes from delta file
 * @param size Number of bytes to read
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
static int read_delta(void * arg_p, uint8_t * buf_p, size_t size)
{
    esp_dota_firm_t * firm = (esp_dota_firm_t *) arg_p;

    if (!firm) {
        ESP_LOGE(TAG, "Error: read_delta - invalid argument");
        return ESP_FAIL;
    }
    if (size <= 0) {
        ESP_LOGE(TAG, "Error: read_delta - invalid size requested");
        return ESP_FAIL;
    }

    if (fread(buf_p, 1, size, firm->delta) != size) {
        ESP_LOGE(TAG, "Error: read_delta - fread(delta)");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Callback to write to the updating partition's image.
 * 
 * @see detools_apply_patch_callbacks()
 * 
 * @param arg_p Pointing to the dota_firm_t passed in detools_apply_patch_callbacks()
 * @param buf_p Buffer storing parts of the new image
 * @param size Number of bytes to write
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
static int write_updating_image(void *arg_p, const uint8_t *buf_p, size_t size)
{
    esp_dota_firm_t * firm = (esp_dota_firm_t *) arg_p;

    if (!firm) {
        ESP_LOGE(TAG, "Error: write_updating_image - invalid argument");
        return ESP_FAIL;
    }
    if (size <= 0) {
        ESP_LOGE(TAG, "Error: write_updating_image - invalid size requested");
        return ESP_FAIL;
    }

    if (esp_ota_write(firm->handle, buf_p, size) != ESP_OK) {
        ESP_LOGE(TAG, "Error: write_updating_image - esp_ota_write()");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t read_appy_delta()
{
    /* Open delta file in read mode */
    dota_firm.delta = fopen(DOTA_SPIFF DOTA_DELTA, "rb");
    if (dota_firm.delta == NULL) {
        ESP_LOGE(TAG, "Error while opening delta file");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Opened delta file to read the patch...");

    /* Apply patch using callbacks */
    int ret = detools_apply_patch_callbacks(
        read_current_image,
        seek_current_image,
        read_delta,
        dota_firm.delta_size,
        write_updating_image,
        &dota_firm
    );

    if (ret <= 0) {
        ESP_LOGE(TAG, "Error: detools_apply_patch_callbacks() - %s", detools_error_as_string(ret));
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t end_ota()
{
    if (esp_ota_end(dota_firm.handle) != ESP_OK) {
        ESP_LOGE(TAG, "Error while end_ota, esp_ota_end failed!");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_set_boot_partition(dota_firm.updating);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error while end_ota, failed to set boot partition err=0x%x", err);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void task_dota()
{
    if (handle_heartbeat(dota_firm.sock) != ESP_OK) {
        close(dota_firm.sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Receiving delta file...");

    /* Get the delta file and store it */
    if (handle_publish_message(dota_firm.sock, write_delta) != ESP_OK) {
        close(dota_firm.sock);
        vTaskDelete(NULL);
        return;
    }

    close(dota_firm.sock);
    fclose(dota_firm.delta);
    ESP_LOGI(TAG, "Received delta file of %u bytes...", dota_firm.delta_size);
    ESP_LOGI(TAG, "Decompressing and applying patch...");

    /* Read & decompress from the delta file and apply the patches */
    if (read_appy_delta() != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Successfully read and applied update...");

    /* Verify the new firmware image and set the boot partition */
    if (end_ota() != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Patch applied, restarting in 5 seconds...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_restart();
}
