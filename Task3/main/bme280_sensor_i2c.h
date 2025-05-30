#ifndef __BME280_SENSOR_I2C_H__INCLUDED__
#define __BME280_SENSOR_I2C_H__INCLUDED__

#include "driver/i2c_master.h"

#define BME280_SENSOR_ADDR         0x76
#define BME280_SCL_DFLT_FREQ_HZ    100000  

// Puclic Types
typedef int32_t BME280_S32_t;
typedef uint32_t BME280_U32_t;
typedef int64_t BME280_S64_t;

// Sensor mode definitions
typedef enum {
    BME280_SLEEP_MODE = 0,
    BME280_FORCED_MODE = 1,
    BME280_NORMAL_MODE = 3
} bme280_mode_t;

// Oversampling definitions
typedef enum {
    BME280_OVERSAMPLING_SKIPPED = 0,
    BME280_OVERSAMPLING_1X = 1,
    BME280_OVERSAMPLING_2X = 2,
    BME280_OVERSAMPLING_4X = 3,
    BME280_OVERSAMPLING_8X = 4,
    BME280_OVERSAMPLING_16X = 5
} bme280_oversampling_t;

// Filter coefficient definitions
typedef enum {
    BME280_FILTER_OFF = 0,
    BME280_FILTER_COEFF_2 = 1,
    BME280_FILTER_COEFF_4 = 2,
    BME280_FILTER_COEFF_8 = 3,
    BME280_FILTER_COEFF_16 = 4
} bme280_filter_t;

// Standby time definitions (ms)
typedef enum {
    BME280_STANDBY_0_5_MS = 0,
    BME280_STANDBY_62_5_MS = 1,
    BME280_STANDBY_125_MS = 2,
    BME280_STANDBY_250_MS = 3,
    BME280_STANDBY_500_MS = 4,
    BME280_STANDBY_1000_MS = 5,
    BME280_STANDBY_10_MS = 6,
    BME280_STANDBY_20_MS = 7
} bme280_standby_time_t;

// Sensor data structure
typedef struct {
    float temperature;  
    float pressure;     
    float humidity;     
} bme280_data_t;

// Initialize BME280 sensor
void bme280_init(i2c_master_bus_handle_t* pBusHandle,
                i2c_master_dev_handle_t* pSensorHandle,
                int sdaPin, int sclPin, uint32_t clkSpeedHz);

// Free BME280 resources
void bme280_free(i2c_master_bus_handle_t busHandle,
                i2c_master_dev_handle_t sensorHandle);

// Configure BME280 sensor
void bme280_config(i2c_master_dev_handle_t sensorHandle,
                  bme280_mode_t mode,
                  bme280_oversampling_t tempOversampling,
                  bme280_oversampling_t pressOversampling,
                  bme280_oversampling_t humOversampling,
                  bme280_filter_t filter,
                  bme280_standby_time_t standbyTime);

// Read raw compensation data from sensor
void bme280_read_compensation_data(i2c_master_dev_handle_t sensorHandle);

// Read all sensor data (temperature, pressure, humidity)
void bme280_read_data(i2c_master_dev_handle_t sensorHandle, bme280_data_t* data);

// Read only temperature
float bme280_read_temperature(i2c_master_dev_handle_t sensorHandle);

// Read only pressure
float bme280_read_pressure(i2c_master_dev_handle_t sensorHandle);

// Read only humidity
float bme280_read_humidity(i2c_master_dev_handle_t sensorHandle);

// Check if sensor is currently measuring
bool bme280_is_measuring(i2c_master_dev_handle_t sensorHandle);

// Compensation functions
BME280_S32_t BME280_compensate_T_int32(BME280_S32_t adc_T);
BME280_U32_t BME280_compensate_P_int64(BME280_S32_t adc_P);
BME280_U32_t bme280_compensate_H_int32(BME280_S32_t adc_H);


//Compensation conversion functions
float bme280_compensate_T(BME280_S32_t adc_T);
float bme280_compensate_P(BME280_S32_t adc_P);
float bme280_compensate_H(BME280_S32_t adc_H);

#endif // __BME280_SENSOR_I2C_H__INCLUDED__