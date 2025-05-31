#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bme280_sensor_i2c.h"
#include "bme280_sensor_i2c.c"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_littlefs.h"
#include "esp_timer.h"

#define BME280_SCL_IO         3
#define BME280_SDA_IO         2
#define FAN_GPIO              0
#define WIFI_SSID "Potted Plant Temp Control"
#define WIFI_PASSWORD "104341103320"
#define LITTLEFS_BASE_PATH "/littlefs"

static i2c_master_bus_handle_t busHandle;
static i2c_master_dev_handle_t sensorHandle;
static float temperature = 0.0;

//initialize and configer BME280 sensor
void bme280_sensor_init(void) {
    bme280_init(&busHandle, &sensorHandle, BME280_SDA_IO, BME280_SCL_IO, BME280_SCL_DFLT_FREQ_HZ);
    bme280_config(sensorHandle, BME280_NORMAL_MODE, 
                  BME280_OVERSAMPLING_2X, BME280_OVERSAMPLING_4X, BME280_OVERSAMPLING_2X,
                  BME280_FILTER_COEFF_4, BME280_STANDBY_62_5_MS);
}

float read_temperature(void) {
    return bme280_read_temperature(sensorHandle);
}


// Temp handler
esp_err_t bme_handler(httpd_req_t *req) {
    char resp[64];
    snprintf(resp, sizeof(resp), "Temperature: %.2f ºC", temperature);
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

esp_err_t root_handler(httpd_req_t *req){
    const char *html_response = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>Potted Plant Temperature Control</title></head>"
        "<title>Potted Plant Temperature Control</title>"
        "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>"
        "</head>"
        "<body>"
        "<h1>Temperature Monitor</h1>"
        "<canvas id=\"tempChart\" width=\"600\" height=\"300\"></canvas>"
        "<script>"
        "async function fetchLogData() {"
        "  const response = await fetch('/logs');"
        "  const text = await response.text();"
        "  const lines = text.trim().split('\\n');"
        "  const labels = [];"
        "  const data = [];"
        "  lines.forEach(line => {"
        "    const [time, temp] = line.split('|').map(s => s.trim());"
        "    labels.push(time);"
        "    data.push(parseFloat(temp));"
        "  });"
        "  return { labels, data };"
        "}"

        "let chartInstance = null;"

        "async function drawChart() {"
        "  const { labels, data } = await fetchLogData();"
        "  const ctx = document.getElementById('tempChart').getContext('2d');"
        "  if(chartInstance) chartInstance.destroy();"
        "  chartInstance = new Chart(ctx, {"
        "    type: 'line',"
        "    data: { labels: labels, datasets: [{ label: 'Temperature (°C)', data: data, borderColor: 'red', fill: false }] },"
        "    options: {"
        "      scales: {"
        "        x: { title: { display: true, text: 'Time' } },"
        "        y: { title: { display: true, text: 'Temperature (°C)' } }"
        "      }"
        "    }"
        "  });"
        "}"

        "drawChart();"
        "setInterval(drawChart, 4000);"
        "</script>"
        
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_response, strlen(html_response));
    return ESP_OK;
}

esp_err_t log_handler(httpd_req_t *req) {
    FILE *file = fopen(LITTLEFS_BASE_PATH "/temp_log.txt", "r");
    if (!file) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/plain");

    char line[64];
    while (fgets(line, sizeof(line), file)) {
        httpd_resp_send_chunk(req, line, strlen(line));
    }
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// We had add this to avoid favicon.ico missing errors
esp_err_t favicon_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, NULL, 0);  
    return ESP_OK;
}

httpd_uri_t uri_bme = {
    .uri      = "/bme",
    .method   = HTTP_GET,
    .handler  = bme_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_root = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = root_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_logs = {
    .uri      = "/logs",
    .method   = HTTP_GET,
    .handler  = log_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_favicon = {
    .uri      = "/favicon.ico",
    .method   = HTTP_GET,
    .handler  = favicon_handler,
    .user_ctx = NULL
};

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_bme);
        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_favicon);
        httpd_register_uri_handler(server, &uri_logs);
    }
    return server;
}

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
    ESP_LOGI("wifi", "Connect to the Access Point with SSID: %s and Password: %s", WIFI_SSID, WIFI_PASSWORD);
}

void init_nvs(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void mount_littlefs(void){
    esp_vfs_littlefs_conf_t conf = {
        .base_path = LITTLEFS_BASE_PATH,
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE("littlefs", "Mount failed (%s)", esp_err_to_name(err));
    } else {
        ESP_LOGI("littlefs", "Filesystem mounted at /littlefs");
    }
}
void format_time(char *buffer, size_t len){
    int64_t time_us = esp_timer_get_time();
    int seconds = time_us / 1000000;
    int minutes = (seconds / 60) % 60;
    int hours = (seconds / 3600) % 24;
    seconds %= 60;
    snprintf(buffer, len, "%02d:%02d:%02d", hours, minutes, seconds);
}

void log_temp_to_file(float temperature) {
    FILE *file = fopen(LITTLEFS_BASE_PATH "/temp_log.txt", "a");
    if(file){
        char time_str[16];
        format_time(time_str, sizeof(time_str));
        fprintf(file, "%s | %.2f\n",time_str, temperature);
        fclose(file);
    }else {
        ESP_LOGE("littlefs", "Failed to open temp_log.txt for writing");
    }
}

void clear_temp_log_file(void) {
    FILE *file = fopen(LITTLEFS_BASE_PATH "/temp_log.txt", "w");
    if (file) {
        fclose(file);  // Opening in "w" mode truncates the file
        ESP_LOGI("littlefs", "Temperature log file cleared.");
    } else {
        ESP_LOGE("littlefs", "Failed to open temp_log.txt for clearing");
    }
}

void app_main(void)
{
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);

    mount_littlefs();
    clear_temp_log_file();
    init_nvs();
    wifi_init_softap();

    bme280_sensor_init();

    start_webserver();
    while (1) {
        temperature = read_temperature();
        log_temp_to_file(temperature);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
