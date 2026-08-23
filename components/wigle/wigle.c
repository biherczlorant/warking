#include "wigle.h"
#include "gps.h"
#include "sdcard.h"
#include "wifi.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"

static const char *TAG = "wigle";

static int total_unique_aps = 0;

static const char *wigle_auth_to_wigle_string(wifi_auth_mode_t authmode,
                                              wifi_cipher_type_t pairwise) {
  static char buf[40];

  if (authmode == WIFI_AUTH_OPEN) {
    return "[ESS]";
  }

  const char *enc;
  switch (authmode) {
  case WIFI_AUTH_WEP:
    return "[WEP][ESS]";
  case WIFI_AUTH_WPA_PSK:
    enc = "WPA-PSK";
    break;
  case WIFI_AUTH_WPA2_PSK:
    enc = "WPA2-PSK";
    break;
  case WIFI_AUTH_WPA_WPA2_PSK:
    enc = "WPA-WPA2-PSK";
    break;
  case WIFI_AUTH_WPA3_PSK:
    enc = "WPA3-PSK";
    break;
  case WIFI_AUTH_WPA2_WPA3_PSK:
    enc = "WPA2-WPA3-PSK";
    break;
  case WIFI_AUTH_ENTERPRISE:
    enc = "EAP";
    break;
  case WIFI_AUTH_WPA3_ENTERPRISE:
    enc = "WPA3-EAP";
    break;
  case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
    enc = "WPA2-WPA3-EAP";
    break;
  case WIFI_AUTH_OWE:
    enc = "OWE";
    break;
  default:
    enc = "UNKNOWN";
    break;
  }

  const char *cipher;
  switch (pairwise) {
  case WIFI_CIPHER_TYPE_TKIP:
    cipher = "TKIP";
    break;
  case WIFI_CIPHER_TYPE_CCMP:
    cipher = "CCMP";
    break;
  case WIFI_CIPHER_TYPE_TKIP_CCMP:
    cipher = "TKIP+CCMP";
    break;
  case WIFI_CIPHER_TYPE_GCMP:
    cipher = "GCMP";
    break;
  case WIFI_CIPHER_TYPE_GCMP256:
    cipher = "GCMP256";
    break;
  default:
    cipher = "CCMP";
    break;
  }

  snprintf(buf, sizeof(buf), "[%s-%s][ESS]", enc, cipher);
  return buf;
}

static int wigle_channel_to_freq_mhz(uint8_t ch) {
  if (ch >= 1 && ch <= 13) {
    return 2407 + (ch * 5);
  }
  if (ch == 14) {
    return 2484;
  }
  return 0;
}

static const char *wigle_csv_safe_ssid(const uint8_t *ssid) {
  static char out[80];
  const char *s = (const char *)ssid;
  if (s == NULL || s[0] == '\0') {
    out[0] = '\0';
    return out;
  }
  if (strchr(s, ',') == NULL && strchr(s, '"') == NULL) {
    snprintf(out, sizeof(out), "%s", s);
    return out;
  }
  size_t oi = 0;
  out[oi++] = '"';
  for (size_t i = 0; s[i] != '\0' && oi < sizeof(out) - 2; i++) {
    if (s[i] == '"') {
      out[oi++] = '"';
    }
    out[oi++] = s[i];
  }
  out[oi++] = '"';
  out[oi] = '\0';
  return out;
}

int wigle_get_header(char *buf, size_t buf_len) {
  return snprintf(
      buf, buf_len,
      "WigleWifi-1.6,appRelease=1.0,model=WARKINGV1,release=1.0.0,"
      "device=warking,display=none,board=warkingv1,"
      "brand=2hz\n"
      "MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,"
      "CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,"
      "RCOIs,MfgrId,Type\n");
}

int wigle_process(void) {
  wifi_ap_record_t *ap_info = NULL;
  uint16_t ap_count = 0;
  wifi_get_last_scan(&ap_info, &ap_count);

  gps_data_t gps;
  gps_get_curr(&gps);

  if (!gps.valid) {
    ESP_LOGW(TAG, "No GPS fix yet");
    return 0;
  }

  char timestamp[24];
  snprintf(timestamp, sizeof(timestamp), "%02d-%02d-%02d %02d:%02d:%02d",
           gps.year, gps.month, gps.day, gps.hour, gps.min, gps.sec);

  int written = 0;
  char line[192];

  for (int i = 0; i < ap_count; i++) {
    wifi_ap_record_t *ap = &ap_info[i];
    int freq = wigle_channel_to_freq_mhz(ap->primary);
    const char *auth =
        wigle_auth_to_wigle_string(ap->authmode, ap->pairwise_cipher);
    const char *ssid = wigle_csv_safe_ssid(ap->ssid);

    snprintf(line, sizeof(line),
             "%02x:%02x:%02x:%02x:%02x:%02x,%s,%s,%s,%d,%d,%d,"
             "%.8f,%.8f,%d,%.2f,,,WIFI\n",
             ap->bssid[0], ap->bssid[1], ap->bssid[2], ap->bssid[3],
             ap->bssid[4], ap->bssid[5], ssid, auth, timestamp, ap->primary,
             freq, ap->rssi, gps.latitude, gps.longitude,
             (int)lround(gps.altitude_m), gps.accuracy_m);

    if (sdcard_write_row(ap->bssid, line)) {
      ++written;
      ++total_unique_aps;
    }
  }

  ESP_LOGI(TAG, "Logged %d new APs to sdcard", written);
  return written;
}

int wigle_get_total_unique(void) { return total_unique_aps; }
