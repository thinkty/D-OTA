
#ifndef APP_DOTA_H
#define APP_DOTA_H

#include <stdio.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_spiffs.h"

#include "detools.h"

#include "app_pubsub.h"

#ifndef TAG
#define TAG "APP"
#endif

#define DOTA_TOPIC "delta"
#define DOTA_SPIFF "/spiffs" /* Default base path for the virtual file system */
#define DOTA_DELTA "/delta"  /* Path to store the delta file */
#define DOTA_TASK_STACKSIZE (4096) /* Need more stack than default */

typedef struct esp_dota_firm {
    esp_ota_handle_t handle;    // Update handle for OTA related tasks
    const esp_partition_t * updating; // OTA partition to update
    const esp_partition_t * running;  // OTA partition currently running
    size_t running_offset;            // OTA partition offset needed for reading
    size_t ota_size;            // New image size
    int sock;                   // Socket for connecting to pub/sub server
    FILE * delta;               // Delta file
    size_t delta_size;
} esp_dota_firm_t;

/**
 * @brief Check if the OTA partitions are set and initialize the OTA firmware.
 * Also, connect to the pub/sub bridge server and subscribe to the OTA_TOPIC.
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t init_dota();

/**
 * @brief Write the differential data to the preset file so that it can later be
 * used to form the new firmware image.
 * 
 * @param buf Delta file data
 * @param len Length of delta file data
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t write_delta(char * buf, size_t len);

/**
 * @brief Read and decompress the delta file to apply the patches to the
 * appropriate OTA partition.
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t read_appy_delta();

/**
 * @brief Finish  delta OTA update, validate newly written app image, and set
 * boot partition.
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t end_dota();

/**
 * @brief Handle incoming heartbeat messages and publish messages. Apply the
 * updates to the appropriate OTA partition, and on successful write, reset the
 * device to apply the update.
 */
void task_dota();

#endif
