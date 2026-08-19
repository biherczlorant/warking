#include "gps_driver.h"

#include "driver/uart.h"
#include "esp_err.h"
#include <freertos/FreeRTOS.h>

const uart_port_t uart_num = UART_NUM_1;
static QueueHandle_t uart_queue = NULL;
static QueueHandle_t gps_queue = NULL;

static const char *TAG = "gps_driver";

static void gps_driver_read_to_queue(void) {
  int pos = uart_pattern_pop_pos(uart_num);
  uint8_t buffer[128] = {0};
  if (pos != -1) {
    int read_len =
        uart_read_bytes(uart_num, buffer, MIN(pos + 1, sizeof(buffer) - 1),
                        100 / portTICK_PERIOD_MS);
    buffer[read_len] = '\0';
    if (xQueueSend(gps_queue, buffer, 0) != pdTRUE) {
      ESP_LOGW(TAG, "Line queue full, dropping sentence");
    }
  } else {
    ESP_LOGW(TAG, "Pattern Queue Size too small");
    uart_flush_input(uart_num);
  }
}
static void gps_driver_uart_task(void *arg) {
  uart_event_t event;
  while (1) {
    if (xQueueReceive(uart_queue, &event, pdMS_TO_TICKS(200))) {
      switch (event.type) {
      case UART_DATA:
        break;
      case UART_FIFO_OVF:
        ESP_LOGW(TAG, "HW FIFO Overflow");
        uart_flush(uart_num);
        xQueueReset(uart_queue);
        break;
      case UART_BUFFER_FULL:
        ESP_LOGW(TAG, "Ring Buffer Full");
        uart_flush(uart_num);
        xQueueReset(uart_queue);
        break;
      case UART_BREAK:
        ESP_LOGW(TAG, "Rx Break");
        break;
      case UART_PARITY_ERR:
        ESP_LOGE(TAG, "Parity Error");
        break;
      case UART_FRAME_ERR:
        ESP_LOGE(TAG, "Frame Error");
        break;
      case UART_PATTERN_DET:
        gps_driver_read_to_queue();
        break;
      default:
        ESP_LOGW(TAG, "unknown uart event type: %d", event.type);
        break;
      }
    }
  }
  vTaskDelete(NULL);
}

void gps_driver_init(void) {
  uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(
      uart_set_pin(uart_num, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_driver_install(uart_num, 512, 0, 16, &uart_queue, 0));
  uart_enable_pattern_det_baud_intr(uart_num, '\n', 1, 9, 0, 0);
  uart_pattern_queue_reset(uart_num, 16);
  uart_flush(uart_num);

  gps_queue = xQueueCreate(8, 128);

  xTaskCreate(gps_driver_uart_task, "gps_driver", 1024, NULL, 3, NULL);
}

QueueHandle_t gps_driver_get_gps_queue(void) { return gps_queue; }
