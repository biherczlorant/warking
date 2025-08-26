#include "gps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define GPS_UART_NUM UART_NUM_0
#define GPS_BUFFER_SIZE 1024

static const char *TAG = "gps";

static bool parse_gprmc(char *sentence, gps_data_t *gps_data) {
  char *token = strtok(sentence, ",");
  int field = 0;

  while (token != NULL) {
    switch (field) {
    case 1:
      if (strlen(token) >= 6) {
        gps_data->hour = (token[0] - '0') * 10 + (token[1] - '0');
        gps_data->minute = (token[2] - '0') * 10 + (token[3] - '0');
        gps_data->second = (token[4] - '0') * 10 + (token[5] - '0');
      }
      break;
    case 2:
      gps_data->valid = (token[0] == 'A');
      break;
    case 3:
      gps_data->latitude = atof(token) / 100.0;
      break;
    case 5:
      gps_data->longitude = atof(token) / 100.0;
      break;
    case 9:
      if (strlen(token) >= 6) {
        gps_data->day = (token[0] - '0') * 10 + (token[1] - '0');
        gps_data->month = (token[2] - '0') * 10 + (token[3] - '0');
        gps_data->year = 2000 + (token[4] - '0') * 10 + (token[5] - '0');
      }
      break;
    }
    token = strtok(NULL, ",");
    field++;
  }
  return gps_data->valid;
}

static void gps_task(void *parameter) {
  char buffer[GPS_BUFFER_SIZE];
  gps_data_t gps_data = {0};

  while (1) {
    int len = uart_read_bytes(GPS_UART_NUM, (uint8_t *)buffer,
                              GPS_BUFFER_SIZE - 1, 100 / portTICK_PERIOD_MS);

    if (len > 0) {
      buffer[len] = '\0';

      char *line = strtok(buffer, "\r\n");
      while (line != NULL) {
        if (strstr(line, "$GPRMC") == line) {
          if (parse_gprmc(line, &gps_data)) {
            ESP_LOGI(TAG, "Location: %.6f, %.6f\n", gps_data.latitude,
                   gps_data.longitude);
            ESP_LOGI(TAG, "Timestamp: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                   gps_data.year, gps_data.month, gps_data.day, gps_data.hour,
                   gps_data.minute, gps_data.second);
          } else {
            ESP_LOGE(TAG, "Invalid GPS data");
          }
        }
        line = strtok(NULL, "\r\n");
      }
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void gps_init() {
  uart_config_t uart_config = {.baud_rate = 9600,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_param_config(GPS_UART_NUM, &uart_config);
  uart_driver_install(GPS_UART_NUM, GPS_BUFFER_SIZE, GPS_BUFFER_SIZE, 0, NULL,
                      0);
}

void gps_start_task() {
  xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
}
