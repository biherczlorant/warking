#ifndef __GPS_DRIVER_H__
#define __GPS_DRIVER_H__

#include <stdint.h>

void gps_driver_init(void);
int gps_driver_read(uint8_t *out, int len);

#endif // __GPS_DRIVER_H__
