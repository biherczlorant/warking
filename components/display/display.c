#include "display.h"

#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "display";

#define OLED_PIN_SDA 8
#define OLED_PIN_SCL 9
#define OLED_I2C_PORT I2C_NUM_0
#define OLED_I2C_ADDR 0x3C
#define OLED_H_RES 128
#define OLED_V_RES 64
#define OLED_PIXEL_CLK_HZ 400000

lv_disp_t *display_init(void) {
  i2c_master_bus_handle_t i2c_bus = NULL;
  i2c_master_bus_config_t bus_cfg = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = OLED_I2C_PORT,
      .sda_io_num = OLED_PIN_SDA,
      .scl_io_num = OLED_PIN_SCL,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  esp_err_t ret = i2c_new_master_bus(&bus_cfg, &i2c_bus);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
    return NULL;
  }

  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t io_cfg = {
      .dev_addr = OLED_I2C_ADDR,
      .scl_speed_hz = OLED_PIXEL_CLK_HZ,
      .control_phase_bytes = 1,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .dc_bit_offset = 6,
  };
  ret = esp_lcd_new_panel_io_i2c(i2c_bus, &io_cfg, &io_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "esp_lcd_new_panel_io_i2c failed: %s", esp_err_to_name(ret));
    return NULL;
  }

  esp_lcd_panel_ssd1306_config_t ssd1306_cfg = {
      .height = OLED_V_RES,
  };
  esp_lcd_panel_dev_config_t panel_cfg = {
      .bits_per_pixel = 1,
      .reset_gpio_num = -1,
      .vendor_config = &ssd1306_cfg,
  };

  esp_lcd_panel_handle_t panel_handle = NULL;
  ret = esp_lcd_new_panel_ssd1306(io_handle, &panel_cfg, &panel_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "esp_lcd_new_panel_ssd1306 failed: %s", esp_err_to_name(ret));
    return NULL;
  }

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ret = lvgl_port_init(&lvgl_cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "lvgl_port_init failed: %s", esp_err_to_name(ret));
    return NULL;
  }

  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = OLED_H_RES * OLED_V_RES,
      .double_buffer = false,
      .hres = OLED_H_RES,
      .vres = OLED_V_RES,
      .monochrome = true,
      .rotation =
          {
              .swap_xy = false,
              .mirror_x = false,
              .mirror_y = false,
          },
      .flags =
          {
              .buff_dma = true,
              .sw_rotate = false,
          },
  };

  lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
  if (!disp) {
    ESP_LOGE(TAG, "lvgl_port_add_disp failed");
    return NULL;
  }

  ESP_LOGI(TAG, "Display initialized");
  return disp;
}

bool display_lock(uint32_t timeout_ms) { return lvgl_port_lock(timeout_ms); }

void display_unlock(void) { lvgl_port_unlock(); }
