#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"

#define FAN_GPIO 0
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE  
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // duty res 13bits
#define LEDC_FREQUENCY          (4000) // frequency in Hz 4000
#define MAX_BRIGHTNESS_LEVELS   10
#define TEMP_MIN 24.0
#define TEMP_MAX 30.0

static void example_ledc_init(void);

static const char *TAG = "fan_control";

void app_main(void)
{
    example_ledc_init(); // initialize PMW function

    // Configure MOSFET GPIO as an output
    //gpio_reset_pin(FAN_GPIO);
    //gpio_set_direction(FAN_GPIO, GPIO_MODE_OUTPUT);
    //gpio_set_level(FAN_GPIO, 1);
    //ESP_LOGI(TAG, "Fan is ON");

    // simulated temperature values
    float simulated_temps[] = {22.5, 23.8, 24.5, 25.2, 26.7, 28.0, 29.5, 30.2, 31.0, 27.3};
    int temp_count = sizeof(simulated_temps) / sizeof(simulated_temps[0]);
    int temp_index = 0;
    
    while(1) {
        
        float temp = simulated_temps[temp_index];
        temp_index = (temp_index + 1) % temp_count;
        
        int duty = 0;
        if (temp < TEMP_MIN) {
            duty = 0; // 0%
        } else if (temp > TEMP_MAX) {
            duty = 8191; // 100%
        } else {
            duty = (int)(8191 * 0.75); // 75% duty cycle for temperatures between TEMP_MIN and TEMP_MAX
        }
        
        ESP_LOGI(TAG, "Simulated Temp: %.2f °C, PWM Duty: %d", temp, duty);

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

static void example_ledc_init(void)
{
    // prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = FAN_GPIO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}