#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define SDCARD_MOUNT_POINT "/sdcard"

esp_err_t sdcard_init(const char *header, const char *filename);
bool sdcard_write_row(const uint8_t mac[6], const char *line);
void sdcard_deinit(void);

#endif // __SDCARD_H__
