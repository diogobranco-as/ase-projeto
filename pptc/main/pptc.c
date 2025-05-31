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


void app_main(void)
{
    //fan control
    //TBD

    i2c_master_bus_handle_t busHandle;
    i2c_master_dev_handle_t sensorHandle;

    // Initialize BME280 sensor
    bme280_init(&busHandle, &sensorHandle, BME280_SDA_IO, BME280_SCL_IO, BME280_SCL_DFLT_FREQ_HZ);

    // Configure sensor
    bme280_config(sensorHandle, BME280_NORMAL_MODE, 
                  BME280_OVERSAMPLING_2X, BME280_OVERSAMPLING_4X, BME280_OVERSAMPLING_2X,
                  BME280_FILTER_COEFF_4, BME280_STANDBY_62_5_MS);

    while (1) {

        float temperature = bme280_read_temperature(sensorHandle);
        printf("Temperature: %.2f °C\n", 
               temperature);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
