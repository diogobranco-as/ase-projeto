#ifndef LITTLEFS_H
#define LITTLEFS_H

#define LITTLEFS_BASE_PATH "/littlefs"
#include <stdio.h>

void mount_littlefs(void);
void format_time(char *buffer, size_t len);
void log_temp_to_file(float temperature);
void clear_temp_log_file(void);

#endif 