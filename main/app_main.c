
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "app_task_dht.h"
#include "app_task_cds.h"
#include "app_ota.h"

esp_ota_firm_t ota_firm;

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // Create event loop to handle TCP events in various tasks
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize OTA-update
    // ESP_ERROR_CHECK(init_ota());

    // Initialize and run the temperature/humidity sensor task
    ESP_ERROR_CHECK(init_dht());
    xTaskCreate(task_dht, "task_dht", DEFAULT_THREAD_STACKSIZE, NULL, DEFAULT_THREAD_PRIO, NULL);

    // Initialize and run the light sensor task
    ESP_ERROR_CHECK(init_cds());
    xTaskCreate(task_cds, "task_cds", DEFAULT_THREAD_STACKSIZE, NULL, DEFAULT_THREAD_PRIO, NULL);
}
