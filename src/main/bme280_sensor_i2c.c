#include "bme280_sensor_i2c.h"
#include <math.h>
#include <string.h>

// BME280 register addresses
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_PRESS_LSB 0xF8
#define BME280_REG_PRESS_XLSB 0xF9
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_TEMP_LSB 0xFB
#define BME280_REG_TEMP_XLSB 0xFC
#define BME280_REG_HUM_MSB 0xFD
#define BME280_REG_HUM_LSB 0xFE

// Compensation data registers
#define BME280_REG_DIG_T1 0x88
#define BME280_REG_DIG_T2 0x8A
#define BME280_REG_DIG_T3 0x8C
#define BME280_REG_DIG_P1 0x8E
#define BME280_REG_DIG_P2 0x90
#define BME280_REG_DIG_P3 0x92
#define BME280_REG_DIG_P4 0x94
#define BME280_REG_DIG_P5 0x96
#define BME280_REG_DIG_P6 0x98
#define BME280_REG_DIG_P7 0x9A
#define BME280_REG_DIG_P8 0x9C
#define BME280_REG_DIG_P9 0x9E
#define BME280_REG_DIG_H1 0xA1
#define BME280_REG_DIG_H2 0xE1
#define BME280_REG_DIG_H3 0xE3
#define BME280_REG_DIG_H4 0xE4
#define BME280_REG_DIG_H5 0xE5
#define BME280_REG_DIG_H6 0xE7

// Compensation data structure
typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_compensation_t;

static bme280_compensation_t comp_data;
static int32_t t_fine;

// Initialize BME280 sensor
void bme280_init(i2c_master_bus_handle_t *pBusHandle,
                 i2c_master_dev_handle_t *pSensorHandle,
                 int sdaPin, int sclPin, uint32_t clkSpeedHz)
{
    i2c_master_bus_config_t i2cMasterCfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = sclPin,
        .sda_io_num = sdaPin,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2cMasterCfg, pBusHandle));

    i2c_device_config_t i2cDevCfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_SENSOR_ADDR,
        .scl_speed_hz = clkSpeedHz,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*pBusHandle, &i2cDevCfg, pSensorHandle));

    // Reset the device
    uint8_t reset_cmd = 0xB6;
    ESP_ERROR_CHECK(i2c_master_transmit(*pSensorHandle, &reset_cmd, 1, -1));

    // Read compensation data
    bme280_read_compensation_data(*pSensorHandle);
}

// Free BME280 resources
void bme280_free(i2c_master_bus_handle_t busHandle,
                 i2c_master_dev_handle_t sensorHandle)
{
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(sensorHandle));
    ESP_ERROR_CHECK(i2c_del_master_bus(busHandle));
}

// Configure BME280 sensor
void bme280_config(i2c_master_dev_handle_t sensorHandle,
                   bme280_mode_t mode,
                   bme280_oversampling_t tempOversampling,
                   bme280_oversampling_t pressOversampling,
                   bme280_oversampling_t humOversampling,
                   bme280_filter_t filter,
                   bme280_standby_time_t standbyTime)
{
    // Configure humidity control register
    uint8_t ctrl_hum[2] = {BME280_REG_CTRL_HUM, humOversampling & 0x07};
    ESP_ERROR_CHECK(i2c_master_transmit(sensorHandle, ctrl_hum, 2, -1));

    // Configure measurement control register
    uint8_t ctrl_meas[2] = {BME280_REG_CTRL_MEAS, (tempOversampling << 5) | (pressOversampling << 2) | mode};
    ESP_ERROR_CHECK(i2c_master_transmit(sensorHandle, ctrl_meas, 2, -1));

    // Configure config register
    uint8_t config[2] = {BME280_REG_CONFIG, (standbyTime << 5) | (filter << 2)};
    ESP_ERROR_CHECK(i2c_master_transmit(sensorHandle, config, 2, -1));
}

// Read compensation data from sensor
void bme280_read_compensation_data(i2c_master_dev_handle_t sensorHandle)
{
    uint8_t reg;
    uint8_t data[24];

    // Read temperature and pressure compensation data
    reg = BME280_REG_DIG_T1;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensorHandle, &reg, 1, data, 24, -1));

    comp_data.dig_T1 = (data[1] << 8) | data[0];
    comp_data.dig_T2 = (data[3] << 8) | data[2];
    comp_data.dig_T3 = (data[5] << 8) | data[4];
    comp_data.dig_P1 = (data[7] << 8) | data[6];
    comp_data.dig_P2 = (data[9] << 8) | data[8];
    comp_data.dig_P3 = (data[11] << 8) | data[10];
    comp_data.dig_P4 = (data[13] << 8) | data[12];
    comp_data.dig_P5 = (data[15] << 8) | data[14];
    comp_data.dig_P6 = (data[17] << 8) | data[16];
    comp_data.dig_P7 = (data[19] << 8) | data[18];
    comp_data.dig_P8 = (data[21] << 8) | data[20];
    comp_data.dig_P9 = (data[23] << 8) | data[22];

    // Read H1 compensation data
    reg = BME280_REG_DIG_H1;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensorHandle, &reg, 1, &comp_data.dig_H1, 1, -1));

    // Read H2-H6 compensation data
    reg = BME280_REG_DIG_H2;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensorHandle, &reg, 1, data, 7, -1));

    comp_data.dig_H2 = (data[1] << 8) | data[0];
    comp_data.dig_H3 = data[2];
    comp_data.dig_H4 = (data[3] << 4) | (data[4] & 0x0F);
    comp_data.dig_H5 = (data[5] << 4) | (data[4] >> 4);
    comp_data.dig_H6 = data[6];
}

// Read raw data from sensor
static void bme280_read_raw_data(i2c_master_dev_handle_t sensorHandle,
                                 int32_t *adc_T, int32_t *adc_P, int32_t *adc_H)
{
    uint8_t reg = BME280_REG_PRESS_MSB;
    uint8_t data[8];

    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensorHandle, &reg, 1, data, 8, -1));
    esp_err_t err = i2c_master_transmit_receive(sensorHandle, &reg, 1, data, 8, -1);
    if (err != ESP_OK) {
        printf("I2C read failed: %s\n", esp_err_to_name(err));
        return;
    }

    *adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    *adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    *adc_H = (data[6] << 8) | data[7];
}

// Read all sensor data (temperature, pressure, humidity)
void bme280_read_data(i2c_master_dev_handle_t sensorHandle, bme280_data_t *data)
{
    int32_t adc_T, adc_P, adc_H;

    bme280_read_raw_data(sensorHandle, &adc_T, &adc_P, &adc_H);

    data->temperature = bme280_compensate_T(adc_T);
    data->pressure = bme280_compensate_P(adc_P);
    data->humidity = bme280_compensate_H(adc_H);
}

// Read only temperature
float bme280_read_temperature(i2c_master_dev_handle_t sensorHandle)
{
    int32_t adc_T, adc_P, adc_H;

    bme280_read_raw_data(sensorHandle, &adc_T, &adc_P, &adc_H);
    return bme280_compensate_T(adc_T);
}

// Read only pressure
float bme280_read_pressure(i2c_master_dev_handle_t sensorHandle)
{
    int32_t adc_T, adc_P, adc_H;

    bme280_read_raw_data(sensorHandle, &adc_T, &adc_P, &adc_H);
    bme280_compensate_T(adc_T);
    return bme280_compensate_P(adc_P);
}

// Read only humidity
float bme280_read_humidity(i2c_master_dev_handle_t sensorHandle)
{
    int32_t adc_T, adc_P, adc_H;

    bme280_read_raw_data(sensorHandle, &adc_T, &adc_P, &adc_H);
    bme280_compensate_T(adc_T);
    return bme280_compensate_H(adc_H);
}

// Temperature compensation
BME280_S32_t BME280_compensate_T_int32(BME280_S32_t adc_T)
{
    BME280_S32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((BME280_S32_t)comp_data.dig_T1 << 1))) * ((BME280_S32_t)comp_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((BME280_S32_t)comp_data.dig_T1)) *
              ((adc_T >> 4) - ((BME280_S32_t)comp_data.dig_T1))) >>
             12) *
            ((BME280_S32_t)comp_data.dig_T3)) >>
           14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// Pressure compensation
BME280_U32_t BME280_compensate_P_int64(BME280_S32_t adc_P)
{
    BME280_S64_t var1, var2, p;
    var1 = ((BME280_S64_t)t_fine) - 128000; // Fixed typo in your code (replaced '–' with '-')
    var2 = var1 * var1 * (BME280_S64_t)comp_data.dig_P6;
    var2 = var2 + ((var1 * (BME280_S64_t)comp_data.dig_P5) << 17);
    var2 = var2 + (((BME280_S64_t)comp_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (BME280_S64_t)comp_data.dig_P3) >> 8) +
           ((var1 * (BME280_S64_t)comp_data.dig_P2) << 12);
    var1 = (((((BME280_S64_t)1) << 47) + var1)) * ((BME280_S64_t)comp_data.dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0;
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((BME280_S64_t)comp_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((BME280_S64_t)comp_data.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BME280_S64_t)comp_data.dig_P7) << 4);
    return (BME280_U32_t)p;
}

// Humidity compensation
BME280_U32_t bme280_compensate_H_int32(BME280_S32_t adc_H)
{
    BME280_S32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((BME280_S32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((BME280_S32_t)comp_data.dig_H4) << 20) - 
                  (((BME280_S32_t)comp_data.dig_H5) * v_x1_u32r)) + 
                 ((BME280_S32_t)16384)) >> 15) * 
                (((((((v_x1_u32r * ((BME280_S32_t)comp_data.dig_H6)) >> 10) * 
                   (((v_x1_u32r * ((BME280_S32_t)comp_data.dig_H3)) >> 11) + 
                   ((BME280_S32_t)32768))) >> 10) + 
                  ((BME280_S32_t)2097152)) * 
                 ((BME280_S32_t)comp_data.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                              ((BME280_S32_t)comp_data.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (BME280_U32_t)(v_x1_u32r >> 12);
}

float bme280_compensate_T(BME280_S32_t adc_T) {
    return BME280_compensate_T_int32(adc_T) / 100.0f;  // Convert from 0.01°C to °C
}

float bme280_compensate_P(BME280_S32_t adc_P) {
    return BME280_compensate_P_int64(adc_P) / 256.0f;  // Convert from Q24.8 to hPa
}

float bme280_compensate_H(BME280_S32_t adc_H) {
    return bme280_compensate_H_int32(adc_H) / 1024.0f; // Convert from Q22.10 to %RH
}
