
#ifndef APP_WIFI_H
#define APP_WIFI_H

/**
 * Handle connecting to an access point using the preset SSID and password.
 * 
 * @see https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/examples/wifi/getting_started/station/main/station_example_main.c
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#ifndef TAG
#define TAG "APP"
#endif

/* WiFi configuration */
#define WIFI_SSID          (CONFIG_ESP_WIFI_SSID)
#define WIFI_PASS          (CONFIG_ESP_WIFI_PASSWORD)
#define WIFI_MAXIMUM_RETRY (CONFIG_ESP_WIFI_MAXIMUM_RETRY)

/**
 * The event group allows multiple bits for each event, but only care about two 
 * events for now :
 * 
 * - connected to the AP with an IP
 * - failed to connect after the maximum amount of retries
 */
#define WIFI_CONNECTED_BIT (BIT0)
#define WIFI_FAIL_BIT      (BIT1)

/**
 * @brief Handle the 3 WiFi events:
 * - WiFi stationary mode started : connect to WiFi
 * - Failed to connect or disconnected : retry and if maximum retry reached, exit
 * - Successfully connected : set result to success
 */
void event_handler(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data);

/**
 * @brief Initialize WiFi module and connect using the preconfigured credentials
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t init_wifi();

#endif
