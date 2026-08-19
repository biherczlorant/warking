#include "gps.h"
#include "gps_driver.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <string.h>

static const char *TAG = "gps";

static void gps_process_line(const char *line) {
  if (strstr(line, "$GPGGA") != NULL) {
    ESP_LOGI(TAG, "Received: %s", line);
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
  xTaskCreate(gps_module_task, "gps module task", 1024, NULL, 5, NULL);
}
