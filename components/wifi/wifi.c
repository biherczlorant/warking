#include "wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include <string.h>

#define DEFAULT_SCAN_LIST_SIZE 32

static const char *TAG = "wifi";

static wifi_ap_record_t s_last_scan_ap_info[DEFAULT_SCAN_LIST_SIZE];
static uint16_t s_last_scan_count = 0;

void wifi_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_scan(void) {
  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
  uint16_t ap_count = 0;
  memset(ap_info, 0, sizeof(ap_info));

  esp_wifi_scan_start(NULL, true);

  ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u",
           ap_count, number);

  memcpy(s_last_scan_ap_info, ap_info, sizeof(ap_info));
  s_last_scan_count = number;
}

void wifi_get_last_scan(wifi_ap_record_t **records, uint16_t *count) {
  *records = s_last_scan_ap_info;
  *count = s_last_scan_count;
}
