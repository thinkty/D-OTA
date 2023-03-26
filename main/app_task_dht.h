
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
 * @brief Convert raw GPIO data to meaningful value by packing two data byptes
 * and take into account sign bit.
 * 
 * @param msb Data byte containing the most significant bit.
 * @param lsb Data byte containing the least significant bit.
 * 
 * @returns Signed integer value.
 */
int16_t convert_data(uint8_t msb, uint8_t lsb);

/**
 * @brief Wait for the GPIO pin to change for the given timeout. The timeout is
 * needed for the initialization step and also for data interpretation to
 * indicate 0 or 1.
 * 
 * @param timeout Time to wait.
 * @param expected_state Expected state of the GPIO pin after timeout.
 * @param duration Optional. Used for interpreting data.
 */
bool await_pin_state(uint32_t timeout, bool expected_state, uint32_t * duration);

/**
 * @brief Fetch data from the temperature/humidity sensor.
 * 
 * @see https://github.com/Fonger/ESP8266-RTOS-DHT/blob/master/dht/dht.c
 * 
 * @param bits Boolean array indicating the bits to store the result.
 * 
 * @returns True on successfully data fetched and false on error.
 */
bool read_dht(bool bits[DHT_DATA_BITS]);

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
