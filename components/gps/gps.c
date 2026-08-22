#include "gps.h"
#include "gps_driver.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <math.h>
#include <string.h>

static const char *TAG = "gps";
static gps_data_t curr_gps;

static double last_lat = 0.0;
static double last_lon = 0.0;
static bool have_last_fix = false;
static double distance_m = 0.0;

#define GPS_JITTER_THRESHOLD_M 1.5

static double gps_haversine_m(double lat1, double lon1, double lat2,
                              double lon2) {
  const double R = 6371000.0;
  double dlat = (lat2 - lat1) * M_PI / 180.0;
  double dlon = (lon2 - lon1) * M_PI / 180.0;
  double a = (sin(dlat / 2) * sin(dlat / 2)) +
             (cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
              sin(dlon / 2) * sin(dlon / 2));
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

float gps_get_distance_km(void) { return (float)(s_distance_m / 1000.0); }

static double gps_nmea_to_decimal_longitude(const char *nmea_lon,
                                            char direction) {
  if (!nmea_lon || strlen(nmea_lon) < 4) {
    return 0.0;
  }

  char deg_str[4] = {0};
  int deg_len = (strlen(nmea_lon) > 9) ? 3 : 2;

  strncpy(deg_str, nmea_lon, deg_len);
  double degrees = atof(deg_str);
  double minutes = atof(nmea_lon + deg_len);

  double decimal = degrees + (minutes / 60.0);

  if (direction == 'W' || direction == 'w') {
    decimal *= -1.0;
  }

  return decimal;
}

static double gps_nmea_to_decimal_latitude(const char *nmea_lat,
                                           char direction) {
  if (!nmea_lat || strlen(nmea_lat) < 3) {
    return 0.0;
  }

  char deg_str[3] = {0};

  strncpy(deg_str, nmea_lat, 2);
  double degrees = atof(deg_str);
  double minutes = atof(nmea_lat + 2);

  double decimal = degrees + (minutes / 60.0);

  if (direction == 'S' || direction == 's') {
    decimal *= -1.0;
  }

  return decimal;
}

static void gps_process_line(char *line) {
  char *fields[16] = {0};
  int count = 0;
  char *token;
  if (strstr(line, "$GPGGA") != NULL) {
    ESP_LOGI(TAG, "Received: %s", line);

    while ((token = strsep(&line, ","))) {
      fields[count++] = token;
    }
    if ((atoi(fields[6])) > 0) {
      curr_gps.valid = false; // TODO: change this, dont only set valid to rmc msgs because naming is confusing
      curr_gps.latitude = gps_nmea_to_decimal_latitude(fields[2], *fields[3]);
      curr_gps.longitude = gps_nmea_to_decimal_longitude(fields[4], *fields[5]);
      curr_gps.hdop = atof(fields[8]);
      curr_gps.altitude_m = atof(fields[9]);
      curr_gps.accuracy_m = curr_gps.hdop * 2.5;

      if (have_last_fix) {
        double d = gps_haversine_m(last_lat, last_lon, curr_gps.latitude,
                                   curr_gps.longitude);
        if (d > GPS_JITTER_THRESHOLD_M) {
          distance_m += d;
        }
      }

      last_lat = curr_gps.latitude;
      last_lon = curr_gps.longitude;
      have_last_fix = true;
    }

  } else if (strstr(line, "$GPRMC") != NULL) {
    ESP_LOGI(TAG, "Received: %s", line);

    while ((token = strsep(&line, ","))) {
      fields[count++] = token;
    }
    curr_gps.valid = (fields[2][0] == 'A');

    char time_str[3] = {0};
    memcpy(time_str, fields[1], 2);
    curr_gps.hour = atoi(time_str);
    memcpy(time_str, fields[1] + 2, 2);
    curr_gps.min = atoi(time_str);
    memcpy(time_str, fields[1] + 4, 2);
    curr_gps.sec = atoi(time_str);

    char date_str[3] = {0};
    memcpy(date_str, fields[9], 2);
    curr_gps.day = atoi(date_str);
    memcpy(date_str, fields[9] + 2, 2);
    curr_gps.month = atoi(date_str);
    memcpy(date_str, fields[9] + 4, 2);
    curr_gps.year = 2000 + atoi(date_str);
  }
}

static void gps_module_task(void *arg) {
  QueueHandle_t q = gps_driver_get_gps_queue();
  char line[128] = {0};

  while (1) {
    if (xQueueReceive(q, &line, pdMS_TO_TICKS(500))) {
      gps_process_line(line);
    }
  }
}

void gps_init(void) {
  gps_driver_init();
  xTaskCreate(gps_module_task, "gps", 1024, NULL, 5, NULL);
}

void gps_get_curr(gps_data_t *out) { *out = curr_gps; }
