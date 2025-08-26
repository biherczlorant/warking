#ifndef COMPONENTS_GPS_GPS_H_
#define COMPONENTS_GPS_GPS_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
  float latitude;
  float longitude;
  float altitude;
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  int satellites;
  float hdop;
  bool valid;
} gps_data_t;

void gps_init();
void gps_start_task();

#endif // COMPONENTS_GPS_GPS_H_
