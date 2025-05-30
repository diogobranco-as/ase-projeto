#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bme280_sensor_i2c.h"
#include "bme280_sensor_i2c.c"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include <string.h>

#define BME280_SCL_IO         3
#define BME280_SDA_IO         2
#define FAN_GPIO              0
static const char *TAG = "fan_control";
void app_main(void)
{
    //fan control
    gpio_reset_pin(FAN_GPIO);
    gpio_set_direction(FAN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(FAN_GPIO, 1);
    ESP_LOGI(TAG, "Fan is ON");

    i2c_master_bus_handle_t busHandle;
    i2c_master_dev_handle_t sensorHandle;
    
    // Initialize BME280 sensor
    bme280_init(&busHandle, &sensorHandle, BME280_SDA_IO, BME280_SCL_IO, BME280_SCL_DFLT_FREQ_HZ);

    // Configure sensor
    bme280_config(sensorHandle, BME280_NORMAL_MODE, 
                 BME280_OVERSAMPLING_2X, BME280_OVERSAMPLING_4X, BME280_OVERSAMPLING_2X,
                 BME280_FILTER_COEFF_4, BME280_STANDBY_62_5_MS);
    
    while (1) {
        bme280_data_t data;
        
        bme280_read_data(sensorHandle, &data);
        printf("Data: %.2f °C, %.2f hPa, %.2f %%RH\n", 
               data.temperature, data.pressure/100.0f, data.humidity);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    /*
    while(1) {
        bme280_init(&busHandle, &sensorHandle, BME280_SDA_IO, BME280_SCL_IO, BME280_SCL_DFLT_FREQ_HZ);

        bme280_config(sensorHandle, BME280_NORMAL_MODE, 
                 BME280_OVERSAMPLING_2X, BME280_OVERSAMPLING_4X, BME280_OVERSAMPLING_2X,
                 BME280_FILTER_COEFF_4, BME280_STANDBY_62_5_MS);
    }
    */
    bme280_free(busHandle, sensorHandle);
}