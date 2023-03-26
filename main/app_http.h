
#ifndef APP_HTTP_H
#define APP_HTTP_H

#include "esp_log.h"
#include "esp_http_server.h"
#include "app_ota.h"

#ifndef TAG
#define TAG "APP"
#endif

/**
 * URI handler for root (/) GET request.
 * Respond with the webpage to upload an image.
 */
esp_err_t get_handler(httpd_req_t * req);

/**
 * URI handler for root (/) POST request.
 * Read the binary image from the request.
 * Respond with an adequate page to show the result.
 */
esp_err_t post_handler(httpd_req_t * req);

/**
 * Start the http daemon in background on the default port
 */
void init_http();

#endif
