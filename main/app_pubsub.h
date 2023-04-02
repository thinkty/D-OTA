
#ifndef APP_PUBSUB_H
#define APP_PUBSUB_H

/**
 * Handles communication between the device and the pub/sub broker. For details
 * on the broker application "Bridge", check the link below. In brief summary,
 * first 1-byte is the command (either "P" for publish or "S" for subscribe),
 * followed by the topic that must be 7 bytes. Then, depending on command, data.
 * 
 * @see https://github.com/thinkty/bridge
 */

#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#ifndef TAG
#define TAG "APP"
#endif

#define SERVER_IP     (CONFIG_ESP_BROKER_IP)
#define SERVER_PORT   (CONFIG_ESP_BROKER_PORT)
#define CMD_PUBLISH   ("P")
#define CMD_SUBSCRIBE ("S")
#define CMD_LEN       (1)
#define TOPIC_LEN     (7)
#define MAX_DATA_LEN  (128)
#define CMD_RESP_FAIL ('F')
#define CMD_PUB_MSG_LEN   (2)
#define CMD_PUB_MSG_MAX   (128)
#define CMD_PUB_MSG_END   "\r\n\r\n"

/**
 * @brief Format a message according to the Bridge protocol and handle response.
 * 
 * @param topic Topic to publish to
 * @param data Data to publish
 * @param size Length of data to publish
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t publish(const char * topic, char * data, int size);

/**
 * @brief Send a subscribe request to the Bridge server.
 * 
 * @param topic Topic to subscribe to
 * 
 * @return Socket number on success, and ESP_FAIL or some negative number on
 * failure.
 */
int subscribe(const char * topic);

/**
 * @brief Wait for a heartbeat message and echo back the message.
 * 
 * @param sock Socket to receive the heartbeat message
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t handle_heartbeat(int sock);

/**
 * @brief Read and parse the publish message according to the bridge publish
 * format. After reading, invoke the passed function call to handle the results.
 * 
 * @param sock Socket to receive the publish message
 * @param cb Callback function to invoke with the received message
 * 
 * @return ESP_OK on success, and ESP_FAIL on failure.
 */
esp_err_t handle_publish_message(int sock, esp_err_t (*cb)(char *, size_t));

#endif
