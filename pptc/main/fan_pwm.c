#include "fan_pwm.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

#define FAN_GPIO              0
#define LEDC_TIMER            LEDC_TIMER_0
#define LEDC_MODE             LEDC_LOW_SPEED_MODE  
#define LEDC_CHANNEL          LEDC_CHANNEL_0
#define LEDC_DUTY_RES         LEDC_TIMER_13_BIT // 13-bit resolution
#define LEDC_FREQUENCY        4000 // 4 kHz

static int duty = 0;

void fan_pwm_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES, 
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,             
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = FAN_GPIO,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

void temperature_pwm_control(float temperature, float temp_min, float temp_max) {
    if (temperature < temp_min) {
        duty = 0; 
    } else if (temperature > temp_max) {
        duty = 8191;
    } else {
        duty = (int)((temperature - temp_min) / (temp_max - temp_min) * 8191);
        if(duty < 0) {
            duty = 0; 
        } else if(duty > 8191) {
            duty = 8191; 
        }
    }

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}