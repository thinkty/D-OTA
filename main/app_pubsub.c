
#include "app_pubsub.h"

esp_err_t publish(const char * topic, char * data, int size)
{
    /* Setup socket to connect to the pub/sub broker */
    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    destAddr.sin_family = AF_INET; /* IPv4 */
    destAddr.sin_port = htons(SERVER_PORT);
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    char addr_str[128];
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

    /* Create socket and connect */
    int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    if (connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr)) != 0) {
        close(sock);
        return ESP_FAIL;
    }

    /* Send command */
    int ret = send(sock, CMD_PUBLISH, CMD_LEN, 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "Error (%d) occured during publish (%s)", errno, topic);
        close(sock);
        return ESP_FAIL;
    }

    /* If topic is less than 7 bytes, pad it with spaces */
    char topic_buf[TOPIC_LEN+1] = {0};
    int topic_len = strlen(topic);
    strncpy(topic_buf, topic, topic_len < TOPIC_LEN ? topic_len : TOPIC_LEN);
    for (int i = topic_len; i < TOPIC_LEN; i++) {
        topic_buf[i] = ' ';
    }
    topic_buf[TOPIC_LEN] = '\0';

    /* Send topic */
    ret = send(sock, topic_buf, TOPIC_LEN, 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "Error (%d) occured during publish (%s)", errno, topic);
        close(sock);
        return ESP_FAIL;
    }

    /* Send data */
    ret = send(sock, data, size, 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "Error (%d) occured during publish (%s)", errno, topic);
        close(sock);
        return ESP_FAIL;
    }

    // TODO: what if this causes watchdog timeout?
    
    close(sock);
    return ESP_OK;
}
