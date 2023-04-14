
#include "app_task_cds.h"

void task_cds()
{
    int val;

    for (;;) {

        /* Disable interrupts */
        taskENTER_CRITICAL();
        val = gpio_get_level(CDS_GPIO_PIN);
        taskEXIT_CRITICAL();

        /* Turn on LED if dark and off if bright */
        gpio_set_level(BUILTIN_LED_GPIO, 1-val);

        /* Publish data */
        char data[MAX_DATA_LEN];
        sprintf(data, "light=%s\n", val == 1 ? "Dark" : "Bright");
        if (publish(CDS_PUBLISH_TOPIC, data, strlen(data)) != ESP_OK) {
            break;
        }

        vTaskDelay(CDS_TASK_INTERVAL / portTICK_PERIOD_MS);
    }

    /* Remove this task if an error occurs in the loop */
    ESP_LOGE(TAG, "Failed to read CDS sensor value");
    vTaskDelete(NULL);
}

esp_err_t init_cds()
{
    /* Configuration for the CDS sensor */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << CDS_GPIO_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    if (gpio_config(&io_conf) != ESP_OK ) {
        ESP_LOGE(TAG, "Failed to initialize CDS sensor");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "CDS sensor initialized");

    /* Configure the builtin LED GPIO */
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << BUILTIN_LED_GPIO;

    if (gpio_config(&io_conf) != ESP_OK ) {
        ESP_LOGE(TAG, "Failed to initialize builtin LED");
        return ESP_FAIL;
    }

    /* Need some delay to initialize GPIO */
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Builtin LED initialized");
    return ESP_OK;
}
