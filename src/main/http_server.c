#include "http_server.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_littlefs.h"

#define LITTLEFS_BASE_PATH "/littlefs"

esp_err_t root_handler(httpd_req_t *req) {
    const char *html_response =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>Potted Plant Temperature Control</title></head>"
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
        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_favicon);
        httpd_register_uri_handler(server, &uri_logs);
    }
    return server;
}
