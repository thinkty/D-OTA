
#include "app_tasks.h"

static inline int16_t th_convert_data(uint8_t msb, uint8_t lsb)
{
    int16_t data = msb & 0x7F;
    data <<= 8;
    data |= lsb;

    // Convert it to negative if most significant bit is 1
    if (msb & BIT(7)) {
        return 0-data; 
    }
    return data;
}

static bool th_await_pin_state(uint8_t pin, uint32_t timeout, bool expected_state, uint32_t * duration)
{
    for (uint32_t i = 0; i < timeout; i += DHT_TIMER_INTERVAL) {
        // Need to wait at least a single interval to prevent reading a jitter
        os_delay_us(DHT_TIMER_INTERVAL);
        if (gpio_get_level(pin) == expected_state) {
            if (duration) {
                *duration = i;
            }
            return true;
        }
    }

    return false;
}

static inline bool th_fetch_data(uint8_t pin, bool bits[DHT_DATA_BITS])
{
    // Phase 'A' pulling signal low to initiate read sequence
    // @see https://github.com/Fonger/ESP8266-RTOS-DHT/blob/master/dht/dht.c
    gpio_set_level(pin, 0);
    os_delay_us(20000);
    gpio_set_level(pin, 1);

    // Phase 'B', 40us
    if (!th_await_pin_state(pin, 40, false, NULL)) {
        return false;
    }

    // Phase 'C', 88us
    if (!th_await_pin_state(pin, 88, true, NULL)) {
        return false;
    }

    // Phase 'D', 88us
    if (!th_await_pin_state(pin, 88, false, NULL)) {
        return false;
    }

    // Read in each of the 40 bits of data...
    uint32_t low, high;
    for (int i = 0; i < DHT_DATA_BITS; i++) {
        if (!th_await_pin_state(pin, 65, true, &low)) {
            return false;
        }
        if (!th_await_pin_state(pin, 75, false, &high)) {
            return false;
        }
        bits[i] = high > low;
    }
    return true;
}

void th_task(void * arg)
{
    // Parse the arguments
    task_args args = *(task_args *) arg;
    bool bits[DHT_DATA_BITS], res;
    uint8_t data[DHT_DATA_BITS/8] = {0};
    int16_t humidity, temperature;

    // Infinite loop to read/print temperature and humidity
    while (1) {

        // Disable interrupts
        taskENTER_CRITICAL();
        res = th_fetch_data(args.pin, bits);
        taskEXIT_CRITICAL();

        if (!res) {
            ESP_LOGE(TAG, "TH_TASK : Failed to fetch data from GPIO(%u)", args.pin);
            break;
        }

        // Read each bit into byte array
        for (uint8_t i = 0; i < DHT_DATA_BITS; i++) {
            data[i/8] <<= 1;
            data[i/8] |= bits[i];
        }

        if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
            ESP_LOGE(TAG, "TH_TASK : Checksum failed GPIO(%u)", args.pin);
            break;
        }

        humidity = th_convert_data(data[0], data[1]) / 10;
        temperature = th_convert_data(data[2], data[3]) / 10;

        ESP_LOGI(TAG, "TH_TASK : humidity=%d%%, temperature=%d°C", humidity, temperature);

        vTaskDelay(args.interval / portTICK_PERIOD_MS);
    }

    // Remove this task if an error occurs in the loop
    ESP_LOGE(TAG, "TH_TASK : Deleting task");
    vTaskDelete(NULL);
}

esp_err_t init_th(uint8_t pin)
{
    ESP_LOGI(TAG, "Initializing GPIO(%u) for temperature/humidity sensor", pin);
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pin_bit_mask = 1ULL << pin,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    // Configure GPIO with the given settings
    if (gpio_config(&io_conf) == ESP_OK ) {
        gpio_set_level(pin, 1);

        // Need some delay to initialize GPIO
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "GPIO(%u) initialized", pin);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to initialize temperature/humidity sensor on GPIO(%u)", pin);
    return ESP_FAIL;
}


void ll_task(void * arg)
{
    // Parse the arguments
    task_args args = *(task_args *) arg;
    int val;

    // Infinite loop to read light and toggle LED
    while (1) {

        // Disable interrupts
        taskENTER_CRITICAL();

        // Returns 1 if bright and 0 if dark
        val = gpio_get_level(args.pin);
        taskEXIT_CRITICAL();

        ESP_LOGI(TAG, "LL_TASK : %s", val == 1 ? "Dark" : "Bright");
        
        // Turn on LED if dark and off if bright
        gpio_set_level(BUILTIN_LED_GPIO, val == 1 ? 0 : 1);

        vTaskDelay(args.interval / portTICK_PERIOD_MS);
    }

    // Remove this task if an error occurs in the loop
    ESP_LOGE(TAG, "LL_TASK : Deleting task");
    vTaskDelete(NULL);
}

esp_err_t init_ll(uint8_t pin)
{
    ESP_LOGI(TAG, "Initializing GPIO(%u) for light sensor", pin);
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << pin,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    // Configure light sensor GPIO with the given settings 
    if (gpio_config(&io_conf) != ESP_OK ) {
        ESP_LOGE(TAG, "Failed to initialize light sensor on GPIO(%u)", pin);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "GPIO(%u) initialized", pin);

    // Configure builtin LED GPIO
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << BUILTIN_LED_GPIO;
    if (gpio_config(&io_conf) != ESP_OK ) {
        ESP_LOGE(TAG, "Failed to initialize builtin LED on GPIO(%u)", BUILTIN_LED_GPIO);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "GPIO(%u) initialized", BUILTIN_LED_GPIO);

    // Need some delay to initialize GPIO
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    return ESP_OK;
}
