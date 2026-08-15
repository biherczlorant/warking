#include "gps_driver.h"

#include "driver/uart.h"
#include "esp_err.h"

const uart_port_t uart_num = UART_NUM_1;

static const char *TAG = "gps_driver";

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
  ESP_ERROR_CHECK(uart_driver_install(uart_num, 1024, 0, 0, NULL, 0));
}

int gps_driver_read(uint8_t *out, int len) {
  return uart_read_bytes(uart_num, out, len, 100 / portTICK_PERIOD_MS);
}
