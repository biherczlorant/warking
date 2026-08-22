#include "display.h"
#include "esp_log.h"
#include "gps.h"
#include "sdcard.h"
#include "ui.h"
#include "wifi.h"
#include "wigle.h"

#include "nvs_flash.h"
#include <freertos/FreeRTOS.h>

static const char *TAG = "main";

void main_task(void *arg) {
  while (1) {
    wifi_scan();
    wigle_process();

    int total_aps = wigle_get_total_unique();
    float distance_km = gps_get_distance_km();

    if (display_lock(100)) {
      lv_label_set_text_fmt(ui_Laps, "APs: %d", total_aps);
      lv_label_set_text_fmt(ui_Ldist, "Dist: %.1f km", distance_km);
      display_unlock();
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  char header[256];
  wigle_get_header(header, sizeof(header));
  ESP_ERROR_CHECK(sdcard_init(header, "wigle.txt"));
  wifi_init();
  gps_init();

  lv_disp_t *disp = display_init();
  if (disp) {
    if (display_lock(portMAX_DELAY)) {
      ui_init();
      display_unlock();
    }
  }

  xTaskCreate(main_task, "main_task", configMINIMAL_STACK_SIZE * 4, NULL, 4,
              NULL);
}
