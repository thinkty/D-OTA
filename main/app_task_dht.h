
#ifndef APP_TASK_DHT_H
#define APP_TASK_DHT_H

/**
 * @see https://github.com/Fonger/ESP8266-RTOS-DHT
 * @see https://github.com/espressif/ESP8266_RTOS_SDK/tree/master/examples/peripherals/gpio
 * 
 * Most of the basic GPIO setup for the ESP8266 RTOS SDK was from the above
 * repositories.
 */

#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_pubsub.h"

#ifndef TAG
#define TAG "APP"
#endif

#define DHT_TIMER_INTERVAL (2)    /* DHT timer precision in microseconds */
#define DHT_DATA_BITS      (40)
#define DHT_TASK_INTERVAL  (5000) /* Task interval (in milliseconds) */
#define DHT_GPIO_PIN       (4)    /* GPIO pin for the sensor */
#define DHT_PUBLISH_TOPIC  ("dht22")

/**
 * @brief Read and print temperature and humidity from the sensor. Send data to
 * the pub/sub broker.
 */
void task_dht();

/**
 * @brief Initialize the GPIO for temperature and humidity sensor (AM2302 or
 * DHT22).
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t init_dht();

#endif
