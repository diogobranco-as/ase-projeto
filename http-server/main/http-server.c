#include <stdio.h>
#include <stdlib.h>
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#define WIFI_SSID "PtempControl"
#define WIFI_PASSWORD "104341103320"


// NEEDS TO BE ACTUAL BME280 SENSOR DATA
static float temperature = 0.0;

void update_sensor_data(void *pvParameters) {
    while(1) {
        temperature = (float)(rand() % 300) / 10.0f;
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req) {
    // Send a simple response
    const char resp[] = "Hello World! URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Our URI handler function to be called during POST /uri request */
esp_err_t post_handler(httpd_req_t *req) {
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t bme_handler(httpd_req_t *req){
    char resp[512];  // Increased from 100 to 512 bytes
    snprintf(resp, sizeof(resp),
            "<html>"
            "<head><meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<meta http-equiv='refresh' content='2'></head>"
            "<body style='font-family: Arial; text-align: center;'>"
            "<h1>BME280 Sensor Data</h1>"
            "<p style='font-size: 24px;'>Temperature: <strong>%.1f °C</strong></p>"
            "<p><a href='/uri'>Back to main</a></p>"
            "</body>"
            "</html>", 
            temperature);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

httpd_uri_t uri_get = {
    .uri      = "/uri",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post = {
    .uri      = "/uri",
    .method   = HTTP_POST,
    .handler  = post_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_bme = {
    .uri      = "/bme",
    .method   = HTTP_GET,
    .handler  = bme_handler,
    .user_ctx = NULL
};

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_bme);
    }
    return server;
}

/* WiFi Access Point Setup */
void wifi_init_softap(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD, 
            .ssid_len = strlen(WIFI_SSID),
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    esp_wifi_start();

    printf("WiFi AP started. SSID: %s Password: %s\n", 
           wifi_config.ap.ssid, wifi_config.ap.password);
}

void app_main(void) {
    nvs_flash_erase();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    
    wifi_init_softap();
    wifi_config_t conf;
    esp_wifi_get_config(ESP_IF_WIFI_AP, &conf);
    printf("Connect to Wifi %s with Password: %s\n", conf.ap.ssid, conf.ap.password);
    // Start web server
    httpd_handle_t server = start_webserver();
    if (server) {
        printf("Web server started!\n");
        printf("http://192.168.4.1/uri\n");
    }
    xTaskCreate(update_sensor_data, "update_sensor_data", 2048, NULL, 5, NULL);
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}