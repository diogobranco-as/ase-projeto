#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bme280_sensor_i2c.h"
#include "soft_ap.h"
#include "fan_pwm.h"
#include "http_server.h"
#include "littlefs.h"
#include "esp_log.h"

#define BME280_SCL_IO         3
#define BME280_SDA_IO         2

#define LITTLEFS_BASE_PATH "/littlefs"
#define TEMP_MIN 20.0
#define TEMP_MAX 27.0

static i2c_master_bus_handle_t busHandle;
static i2c_master_dev_handle_t sensorHandle;
static float temperature = 0.0;

void bme280_sensor_init(void) {
    bme280_init(&busHandle, &sensorHandle, BME280_SDA_IO, BME280_SCL_IO, BME280_SCL_DFLT_FREQ_HZ);
    bme280_config(sensorHandle, BME280_NORMAL_MODE, 
                  BME280_OVERSAMPLING_2X, BME280_OVERSAMPLING_4X, BME280_OVERSAMPLING_2X,
                  BME280_FILTER_COEFF_4, BME280_STANDBY_62_5_MS);
}

void app_main(void)
{
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);

    mount_littlefs();
    clear_temp_log_file();
    wifi_init_softap();

    bme280_sensor_init();

    start_webserver();
    fan_pwm_init(); 

    while (1) {
        temperature = bme280_read_temperature(sensorHandle);
        log_temp_to_file(temperature);
        temperature_pwm_control(temperature, TEMP_MIN, TEMP_MAX);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
