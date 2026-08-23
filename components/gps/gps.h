#ifndef __GPS_H__
#define __GPS_H__

#include <stdbool.h>

typedef struct {
  double latitude, longitude, altitude_m, hdop, accuracy_m;
  int hour, min, sec, day, month, year;
  bool valid;
} gps_data_t;

void gps_init(void);
void gps_get_curr(gps_data_t *out);
float gps_get_distance_km(void);

#endif //__GPS_H__
