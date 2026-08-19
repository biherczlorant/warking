#include "esp_log.h"
#include "gps.h"
#include "wifi.h"

#include "nvs_flash.h"
#include <freertos/FreeRTOS.h>

static const char *TAG = "main";

void main_task(void *arg) {
  while (1) {
    wifi_scan();
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
  wifi_init();
  gps_init();

  xTaskCreate(main_task, "main_task", configMINIMAL_STACK_SIZE * 4, NULL, 4,
              NULL);
}
