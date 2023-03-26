
#ifndef APP_OTA_H
#define APP_OTA_H

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

#ifndef TAG
#define TAG "APP"
#endif

typedef struct esp_ota_firm {
    esp_ota_handle_t handle; // Update handle for OTA related tasks
    esp_partition_t * updating; // OTA partition to update
    size_t ota_size; // Image size
} esp_ota_firm_t;

/**
 * @see https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/examples/system/ota/native_ota/2%2BMB_flash/new_to_new_no_old/main/ota_example_main.c
 */

/**
 * @brief Check if the OTA partitions are set and initialize the OTA firmware.
 * 
 * @param ota_firm Struct to store OTA related info (OTA partition, handle, etc.)
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t init_ota(esp_ota_firm_t * ota_firm);

/**
 * @brief Write OTA update data to partition. Data is written sequentially to
 * the partition.
 * 
 * @param ota_firm Struct to store OTA related info (OTA partition, size, offest, etc.)
 * @param buf Image data
 * @param len Length of image data
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t write_ota(esp_ota_firm_t * ota_firm, char * buf, size_t len);

/**
 * @brief Finish OTA update, validate newly written app image, and set boot
 * partition.
 * 
 * @param ota_firm Struct to store OTA related info (OTA partition, size, offest, etc.)
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t end_ota(esp_ota_firm_t * ota_firm);

#endif
