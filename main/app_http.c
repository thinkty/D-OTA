
#include "app_http.h"

static const char * INDEX_PAGE = 
"<!DOCTYPE html><html lang='en'>"
"<head>"
"<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta http-equiv='X-UA-Compatible' content='ie=edge'>"
"<title>OTA Update</title>"
"<style>body {background-color: #c4c4c4; font-family: Arial, Helvetica, sans-serif; margin: 0; padding: 0; height: 100vh; display: flex; flex-direction: column; justify-content: center; align-items: center; } h1 { padding: 1em 1em 0 1em; margin: 0; } h3 { margin: 0 0 2em 0; } svg { width: 10em; height: auto; } form { display: flex; flex-direction: column; justify-content: center; align-items: center; gap: 1em; } input, button { cursor: pointer; font-size: large; } #btn-restart { display: none; }</style>"
"<script>"
"function submit() { var file = document.getElementById('input-file').files[0]; var xhr = new XMLHttpRequest(); xhr.onreadystatechange = function () { document.getElementById('upload-response').innerHTML = xhr.responseText; if (this.readyState == 4 && this.status == 200) { document.getElementById('btn-restart').style.display = 'block'; } }; xhr.open('POST', '/');xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');xhr.setRequestHeader('X-File-Name', file.name);xhr.setRequestHeader('Content-Type', file.type || 'application/octet-stream');xhr.send(file); }"
"function restart() { var xhr = new XMLHttpRequest(); xhr.onreadystatechange = function () { if (this.readyState == 4 && this.status == 200) { window.location.reload(); } }; xhr.open('PUT', '/'); xhr.send(); }"
"</script>"
"</head>"
"<body>"
"<div><svg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 24 24'><path xmlns='http://www.w3.org/2000/svg'd='M11 4C8.79082 4 7 5.79085 7 8C7 8.03242 7.00047 8.06627 7.00131 8.10224C7.01219 8.56727 6.70099 8.97839 6.25047 9.09416C4.95577 9.42685 4 10.6031 4 12C4 13.6569 5.34317 15 7 15H8C8.55228 15 9 15.4477 9 16C9 16.5523 8.55228 17 8 17H7C4.23861 17 2 14.7614 2 12C2 9.93746 3.2482 8.16845 5.02926 7.40373C5.32856 4.36995 7.88746 2 11 2C13.2236 2 15.1629 3.20934 16.199 5.00324C19.4207 5.10823 22 7.75289 22 11C22 14.3137 19.3138 17 16 17C15.4477 17 15 16.5523 15 16C15 15.4477 15.4477 15 16 15C18.2092 15 20 13.2091 20 11C20 8.79085 18.2092 7 16 7C15.8893 7 15.78 7.00447 15.6718 7.01322C15.2449 7.04776 14.8434 6.8066 14.6734 6.4135C14.0584 4.99174 12.6439 4 11 4ZM11.2929 9.29289C11.6834 8.90237 12.3166 8.90237 12.7071 9.29289L14.7071 11.2929C15.0976 11.6834 15.0976 12.3166 14.7071 12.7071C14.3166 13.0976 13.6834 13.0976 13.2929 12.7071L13 12.4142V20C13 20.5523 12.5523 21 12 21C11.4477 21 11 20.5523 11 20V12.4142L10.7071 12.7071C10.3166 13.0976 9.68342 13.0976 9.29289 12.7071C8.90237 12.3166 8.90237 11.6834 9.29289 11.2929L11.2929 9.29289Z'fill='black'></path></svg></div>"
"<h1>Over-the-Air Update</h1><h3>ESP8266</h3>"
"<input id='input-file' type='file' accept='' />"
"<button onclick='submit();'>Upload</button>"
"<p id='upload-response'></p>"
"<button id='btn-restart' onclick='restart()'>Restart</button>"
"</body></html>";

static const char * UPLOAD_SUCCESS = "Successfully uploaded image";
static const char * UPLOAD_FAIL = "Failed to upload image";

extern esp_ota_firm_t ota_firm;

esp_err_t get_handler(httpd_req_t * req)
{
    httpd_resp_send(req, INDEX_PAGE, strlen(INDEX_PAGE));

    return ESP_OK;
}

esp_err_t restart_handler(httpd_req_t * req)
{
    httpd_resp_send(req, INDEX_PAGE, strlen(INDEX_PAGE));

    ESP_LOGI(TAG, "Prepare to restart system in 3!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Prepare to restart system in 2!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Prepare to restart system in 1!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
}

esp_err_t post_handler(httpd_req_t * req)
{

    ESP_LOGI(TAG, "Received file upload %d bytes", req->content_len);

    // Basic check
    if (req->method != HTTP_POST) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, UPLOAD_FAIL, strlen(UPLOAD_FAIL));
        return ESP_OK;
    }

    char buf[100];
    size_t ret = 0, remaining = req->content_len;
    while (remaining > 0) {
        // Read data from request
        ret = httpd_req_recv(req, buf, remaining < sizeof(buf) ? remaining : sizeof(buf));

        // Retry receiving if timeout occurred
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        } else if (ret <= 0) {
            return ESP_FAIL;
        }

        // Write read bytes into the OTA partition
        if (write_ota(&ota_firm, buf, ret) != ESP_OK) {
            return ESP_FAIL;
        }

        remaining -= ret;
    }

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, UPLOAD_SUCCESS, strlen(UPLOAD_SUCCESS));

    // End ota_firm and restart
    if (end_ota(&ota_firm) != ESP_OK) {
        ESP_LOGE(TAG, "Error: end_ota");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void init_http()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server on default port
    ESP_LOGI(TAG, "Starting HTTP server on port: '%d'", config.server_port);
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    // URI handler for root
    httpd_uri_t home = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &home);

    // URI handle for upload
    httpd_uri_t upload = {
        .uri       = "/",
        .method    = HTTP_POST,
        .handler   = post_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &upload);

    // URI handle for restart
    httpd_uri_t restart = {
        .uri       = "/",
        .method    = HTTP_PUT,
        .handler   = restart_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &restart);
}
