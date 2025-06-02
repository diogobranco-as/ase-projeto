#include "littlefs.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_timer.h"

void mount_littlefs(void){
    esp_vfs_littlefs_conf_t conf = {
        .base_path = LITTLEFS_BASE_PATH,
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE("littlefs", "Mount failed (%s)", esp_err_to_name(err));
    } else {
        ESP_LOGI("littlefs", "Filesystem mounted at /littlefs");
    }
}
void format_time(char *buffer, size_t len){
    int64_t time_us = esp_timer_get_time();
    int seconds = time_us / 1000000;
    int minutes = (seconds / 60) % 60;
    int hours = (seconds / 3600) % 24;
    seconds %= 60;
    snprintf(buffer, len, "%02d:%02d:%02d", hours, minutes, seconds);
}

void log_temp_to_file(float temperature) {
    FILE *file = fopen(LITTLEFS_BASE_PATH "/temp_log.txt", "a");
    if(file){
        char time_str[16];
        format_time(time_str, sizeof(time_str));
        fprintf(file, "%s | %.2f\n",time_str, temperature);
        fclose(file);
    }else {
        ESP_LOGE("littlefs", "Failed to open temp_log.txt for writing");
    }
}

void clear_temp_log_file(void) {
    FILE *file = fopen(LITTLEFS_BASE_PATH "/temp_log.txt", "w");
    if (file) {
        fclose(file);  
        ESP_LOGI("littlefs", "Temperature log file cleared.");
    } else {
        ESP_LOGE("littlefs", "Failed to open temp_log.txt for clearing");
    }
}