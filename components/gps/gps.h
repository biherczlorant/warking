#ifndef __GPS_H__
#define __GPS_H__

typedef struct {
  double latitude, longitude, altitude_m, hdop, accuracy_m;
  int fix_quality, satellites;
  int hour, min, sec, day, month, year;
  bool valid;
} gps_data_t;

void gps_init(void);

#endif //__GPS_H__
