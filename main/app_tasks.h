
#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

#ifndef TAG
#define TAG "APP"
#endif

// DHT timer precision in microseconds
#define DHT_TIMER_INTERVAL (2)
#define DHT_DATA_BITS      (40)
#define BUILTIN_LED_GPIO   (GPIO_NUM_16)

/**
 * @see https://github.com/Fonger/ESP8266-RTOS-DHT
 * @see https://github.com/espressif/ESP8266_RTOS_SDK/tree/master/examples/peripherals/gpio
 * 
 * Most of the basic GPIO setup for the ESP8266 RTOS SDK was from the above
 * repositories.
 */

/**
 * @brief Struct for the th_task function argument. Contains the interval and
 * the pin for the GPIO.
 */
typedef struct {
    uint16_t interval;
    uint8_t pin;
} task_args;

/**
 * @brief Convert raw GPIO data to meaningful value by packing two data byptes
 * and take into account sign bit.
 * 
 * @param msb Data byte containing the most significant bit.
 * @param lsb Data byte containing the least significant bit.
 * 
 * @returns Signed integer value.
 */
static inline int16_t th_convert_data(uint8_t msb, uint8_t lsb);

/**
 * @brief Wait for the GPIO pin to change for the given timeout. The timeout is
 * needed for the initialization step and also for data interpretation to
 * indicate 0 or 1.
 * 
 * @param pin GPIO pin number.
 * @param timeout Time to wait.
 * @param expected_state Expected state of the GPIO pin after timeout.
 * @param duration Optional. Used for interpreting data.
 */
static bool th_await_pin_state(uint8_t pin, uint32_t timeout, bool expected_state, uint32_t * duration);

/**
 * @brief Fetch data from the temperature/humidity sensor. Static and inline
 * used since it's the only file used in and it needs to be done ASAP since
 * it's a critical section.
 * 
 * @param pin GPIO pin number.
 * @param bits Boolean array indicating the bits to store the result.
 * 
 * @returns True on successfully data fetched and false on error.
 */
static inline bool th_fetch_data(uint8_t pin, bool bits[DHT_DATA_BITS]);

/**
 * @brief Read and print temperature and humidity from the sensor.
 * 
 * @param arg Points to the interval for fetching and GPIO pin number.
 */
void th_task(void * arg);

/**
 * @brief Initialize the GPIO for temperature and humidity sensor
 * (AM2302 or DHT22).
 * 
 * @param pin GPIO data pin number for the sensor.
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t init_th(uint8_t pin);

/**
 * @brief Read from the light sensor and toggle the builtin LED based on value.
 * 
 * @param arg Points to the interval for fetching and GPIO pin number.
 */
void ll_task(void * arg);

/**
 * @brief Initialize the GPIO for light sensor (SZH-SSBH-011) and the built-in
 * LED for the NodeMCU board.
 * 
 * @param pin GPIO data pin number for the sensor.
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t init_ll(uint8_t pin);

#endif
