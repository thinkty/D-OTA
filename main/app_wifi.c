
#include "app_wifi.h"

/* FreeRTOS event group to signal when connected*/
static EventGroupHandle_t s_wifi_event_group;

/* Keeping track of AP connection retry */
static int s_retry_num = 0;

void event_handler(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
    /* Stationary mode started */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    /* Failed to connect to or disconnected from WiFi */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to WiFi...");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG,"Failed to connect to WiFi...");
        }
        return;
    }

    /* Successful connection and received IP address */
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t * event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected to WiFi... [ IP : %s ]", ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        return;
    }
}

esp_err_t init_wifi(void)
{
    s_wifi_event_group = xEventGroupCreate();

    /* Initialize WiFi module */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        return ESP_FAIL;
    }

    /* Add event handlers */
    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) != ESP_OK ||
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) != ESP_OK) {
        return ESP_FAIL;
    }

    /* Setting password implies station will connect to all security modes   */
    /* including WEP/WPA which is not secure. To specify specific auth mode, */
    /* specify it in the `wifi_config.sta.threshold.authmode` field.         */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    /* Configure and connect */
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Connecting to WiFi...");

    /* Block to wait WiFi connection (whether it succeeds or fails) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    /* Clean-up */
    if (esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler) != ESP_OK ||
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler) != ESP_OK) {
        ESP_LOGI(TAG, "Failed to cleanup event handlers...");
        return ESP_FAIL;
    }
    vEventGroupDelete(s_wifi_event_group);

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    return ESP_FAIL;
}
