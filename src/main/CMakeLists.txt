idf_component_register(SRCS "esp32.c" "littlefs.c" "http_server.c" "fan_pwm.c" "soft_ap.c" "bme280_sensor_i2c.c" 
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi
                    REQUIRES esp_http_server
                    REQUIRES nvs_flash
                    REQUIRES esp_driver_i2c
                    REQUIRES esp_driver_gpio
                    REQUIRES esp_timer
                    REQUIRES lwip
                    REQUIRES esp_driver_ledc)