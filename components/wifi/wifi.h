#ifndef __WIFI_H__
#define __WIFI_H__

#include "esp_wifi.h"

void wifi_scan(void);
void wifi_init(void);
void wifi_get_last_scan(wifi_ap_record_t **records, uint16_t *count);

#endif // __WIFI_H__
