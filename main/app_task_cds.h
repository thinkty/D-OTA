
#ifndef APP_TASK_CDS_H
#define APP_TASK_CDS_H

/**
 * CDS photoresistor sensor measures lights and returns either 1 if it is
 * brighter than the preset value or 0 if it is darker.
 * This task reads the value and turns on the built-in LED if it is dark.
 * The interval is used to 
 * 
 * @see https://github.com/espressif/ESP8266_RTOS_SDK/tree/master/examples/peripherals/gpio
 * 
 * Most of the basic GPIO setup for the ESP8266 RTOS SDK was from the above
 * repositories.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

#ifndef TAG
#define TAG "APP"
#endif

#define CDS_TASK_INTERVAL (5000) /* Task interval (in milliseconds) */
#define CDS_GPIO_PIN      (5) /* GPIO pin for the sensor */
#define BUILTIN_LED_GPIO  (GPIO_NUM_16)

/**
 * @brief Read from the cds sensor and toggle the builtin LED based on value.
 */
void task_cds();

/**
 * @brief Initialize the GPIO for the cds sensor (SZH-SSBH-011) and the built-in
 * LED for the NodeMCU board.
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t init_cds();

#endif
