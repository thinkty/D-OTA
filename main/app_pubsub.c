
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

    /* Send publish command */
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

    /* Handle response */
    char resp;
    if (recv(sock, &resp, sizeof(resp) , 0) <= 0 || resp != 'O') {
        ESP_LOGE(TAG, "Error while receiving response for publish block");
        close(sock);
        return ESP_FAIL;
    }
    
    close(sock);
    return ESP_OK;
}

int subscribe(const char * topic)
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

    /* Send subscribe command */
    int ret = send(sock, CMD_SUBSCRIBE, CMD_LEN, 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "Error (%d) occured during subscribe (%s)", errno, topic);
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

    /* Handle response from the server */
    char resp;
	if (recv(sock, &resp, sizeof(resp), 0) <= 0) {
		ESP_LOGE(TAG, "Error while receiving subscribe request response");
		close(sock);
		return ESP_FAIL;
	}

	/* Server internal error */
	if (resp == CMD_RESP_FAIL) {
		ESP_LOGE(TAG, "Error while subscribing. Server internal error.");
		close(sock);
		return ESP_FAIL;
	}

    return sock;
}

esp_err_t handle_heartbeat(int sock)
{
	/* Wait for heartbeat message */
	char resp;
	if (recv(sock, &resp, sizeof(resp), 0) <= 0) {
        ESP_LOGE(TAG, "Error while receiving heartbeat message");
		return ESP_FAIL;
	}

	/* Reply to heartbeat */
	if (send(sock, &resp, sizeof(resp), 0) <= 0) {
        ESP_LOGE(TAG, "Error while sending heartbeat message");
		return ESP_FAIL;
	}

    return ESP_OK;
}

esp_err_t handle_publish_message(int sock, esp_err_t (*cb)(char *, size_t))
{
    size_t total = 0;

	for (;;) {
		/* First two bytes indicate size */
        char size_buf[CMD_PUB_MSG_LEN];
		if (recv(sock, size_buf, CMD_PUB_MSG_LEN, 0) <= 0) {
			ESP_LOGE(TAG, "Error while receiving size of publish message");
			return ESP_FAIL;
		}

		uint16_t size = ntohs(size_buf[0] << 8 | size_buf[1]);
		uint16_t received = 0;

		/* Make sure to read until the specified size */
        char data_buf[CMD_PUB_MSG_MAX + 1];
		memset(data_buf, 0, CMD_PUB_MSG_MAX + 1);
	    ssize_t ret;
		while (received < size) {
			if ((ret = recv(sock, &data_buf[received], size - received, 0)) <= 0) {
				ESP_LOGE(TAG, "Error while receiving publish message");
				return ESP_FAIL;
			}

			received += ret;
		}

		/* End of stream */
		data_buf[size] = '\0';
		if (strcmp(data_buf, CMD_PUB_MSG_END) == 0) {
			break;
		}

        /* If not the end of stream, invoke the callback function */
        total += received;
        if (cb(data_buf, size) != ESP_OK) {
            ESP_LOGE(TAG, "Error while invoking callback function for the publish message");
            return ESP_FAIL;
        }
	}

    ESP_LOGI(TAG, "Received %u bytes in total...", total);
	return ESP_OK;
}
