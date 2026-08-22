#ifndef __GPS_DRIVER_H__
#define __GPS_DRIVER_H__

#include <freertos/FreeRTOS.h>
#include <stdint.h>

void gps_driver_init(void);
QueueHandle_t gps_driver_get_gps_queue(void);

#endif // __GPS_DRIVER_H__
