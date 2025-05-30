idf_component_register(SRCS "http-server.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi
                    REQUIRES esp_http_server
                    REQUIRES nvs_flash)
