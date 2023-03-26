
#include "app_task_dht.h"

int16_t convert_data(uint8_t msb, uint8_t lsb)
{
    int16_t data = msb & 0x7F;
    data <<= 8;
    data |= lsb;

    /* Convert it to negative if most significant bit is 1 */
    if (msb & BIT(7)) {
        return 0-data; 
    }
    return data;
}

bool await_pin_state(uint32_t timeout, bool expected_state, uint32_t * duration)
{
    for (uint32_t i = 0; i < timeout; i += DHT_TIMER_INTERVAL) {
        /* Need to wait at least a single interval to prevent reading jitter */
        os_delay_us(DHT_TIMER_INTERVAL);
        if (gpio_get_level(DHT_GPIO_PIN) == expected_state) {
            if (duration) {
                *duration = i;
            }
            return true;
        }
    }

    return false;
}

bool read_dht(bool bits[DHT_DATA_BITS])
{
    /* Phase 'A' pulling signal low to initiate read sequence */
    gpio_set_level(DHT_GPIO_PIN, 0);
    os_delay_us(20000);
    gpio_set_level(DHT_GPIO_PIN, 1);

    /* Phase 'B', 40us */
    if (!await_pin_state(40, false, NULL)) {
        return false;
    }

    /* Phase 'C', 88us */
    if (!await_pin_state(88, true, NULL)) {
        return false;
    }

    /* Phase 'D', 88us */
    if (!await_pin_state(88, false, NULL)) {
        return false;
    }

    /* Read in each of the 40 bits of data */
    uint32_t low, high;
    for (int i = 0; i < DHT_DATA_BITS; i++) {
        if (!await_pin_state(65, true, &low)) {
            return false;
        }
        if (!await_pin_state(75, false, &high)) {
            return false;
        }
        bits[i] = high > low;
    }
    return true;
}

void task_dht()
{
    bool bits[DHT_DATA_BITS], res;
    uint8_t data[DHT_DATA_BITS/8] = {0};
    int16_t humidity, temperature;

    for (;;) {

        /* Disable interrupts */
        taskENTER_CRITICAL();
        res = read_dht(bits);
        taskEXIT_CRITICAL();

        /* Failed to read value */
        if (!res) {
            break;
        }

        /* Read each bit into byte array */
        for (uint8_t i = 0; i < DHT_DATA_BITS; i++) {
            data[i/8] <<= 1;
            data[i/8] |= bits[i];
        }

        /* Checksum */
        if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
            break;
        }

        humidity = convert_data(data[0], data[1]) / 10;
        temperature = convert_data(data[2], data[3]) / 10;

        ESP_LOGI(TAG, "humidity=%d%%, temperature=%d°C", humidity, temperature);

        vTaskDelay(DHT_TASK_INTERVAL / portTICK_PERIOD_MS);
    }

    /* Remove this task if an error occurs in the loop */
    ESP_LOGE(TAG, "Failed to read temperature/humidity sensor value");
    vTaskDelete(NULL);
}

esp_err_t init_dht()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pin_bit_mask = 1ULL << DHT_GPIO_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    if (gpio_config(&io_conf) != ESP_OK ) {
        ESP_LOGE(TAG, "Failed to initialize temperature/humidity sensor");
        return ESP_FAIL;
    }

    /* Need some delay to initialize GPIO */
    gpio_set_level(DHT_GPIO_PIN, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Temperature/humidity sensor initialized");
    return ESP_OK;
}
