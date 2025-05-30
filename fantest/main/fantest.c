#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define FAN_GPIO 0  

static const char *TAG = "fan_control";

void app_main(void)
{
    // Configure MOSFET GPIO as an output
    gpio_reset_pin(FAN_GPIO);
    gpio_set_direction(FAN_GPIO, GPIO_MODE_OUTPUT);
    
    // Turn fan ON immediately and permanently
    gpio_set_level(FAN_GPIO, 1);
    
    ESP_LOGI(TAG, "Fan is now ON permanently");
    
    // Keep the task alive (optional but recommended)
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Minimal delay to prevent watchdog trigger
    }
}