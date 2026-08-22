#include "sdcard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "sdcard";

#define SD_PIN_MOSI 6
#define SD_PIN_MISO 5
#define SD_PIN_CLK 4
#define SD_PIN_CS 7
#define SD_SPI_HOST SPI2_HOST

#define SDCARD_DEDUP_CAPACITY 2048
#define SDCARD_EMPTY_SLOT UINT64_MAX

static sdmmc_card_t *s_card = NULL;
static uint64_t *s_dedup_table = NULL;
static char s_file_path[64];
static SemaphoreHandle_t s_lock = NULL;

static uint64_t mac_to_key(const uint8_t mac[6]) {
  uint64_t key = 0;
  for (int i = 0; i < 6; i++) {
    key = (key << 8) | mac[i];
  }
  return key;
}

static bool dedup_contains(uint64_t key) {
  uint32_t idx = (uint32_t)(key % SDCARD_DEDUP_CAPACITY);
  for (uint32_t probes = 0; probes < SDCARD_DEDUP_CAPACITY; probes++) {
    uint64_t slot = s_dedup_table[idx];
    if (slot == SDCARD_EMPTY_SLOT) {
      return false;
    }
    if (slot == key) {
      return true;
    }
    idx = (idx + 1) % SDCARD_DEDUP_CAPACITY;
  }
  return false;
}

static bool dedup_insert(uint64_t key) {
  uint32_t idx = (uint32_t)(key % SDCARD_DEDUP_CAPACITY);
  for (uint32_t probes = 0; probes < SDCARD_DEDUP_CAPACITY; probes++) {
    if (s_dedup_table[idx] == key) {
      return true;
    }
    if (s_dedup_table[idx] == SDCARD_EMPTY_SLOT) {
      s_dedup_table[idx] = key;
      return true;
    }
    idx = (idx + 1) % SDCARD_DEDUP_CAPACITY;
  }
  ESP_LOGW(TAG, "Dedup table full");
  return false;
}

static bool parse_mac_field(const char *field, uint8_t mac[6]) {
  unsigned int b[6];
  if (sscanf(field, "%02x:%02x:%02x:%02x:%02x:%02x", &b[0], &b[1], &b[2], &b[3],
             &b[4], &b[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; i++) {
    mac[i] = (uint8_t)b[i];
  }
  return true;
}

static void preload_existing_macs(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    return;
  }

  char line[256];
  int loaded = 0;
  while (fgets(line, sizeof(line), f)) {
    char *comma = strchr(line, ',');
    if (!comma) {
      continue;
    }
    *comma = '\0';
    uint8_t mac[6];
    if (parse_mac_field(line, mac)) {
      dedup_insert(mac_to_key(mac));
      loaded++;
    }
  }
  fclose(f);
  ESP_LOGI(TAG, "Preloaded %d existing MACs from %s for dedup", loaded, path);
}

esp_err_t sdcard_init(const char *header, const char *filename) {
  esp_err_t ret;

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = SD_PIN_MOSI,
      .miso_io_num = SD_PIN_MISO,
      .sclk_io_num = SD_PIN_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };
  ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
    return ret;
  }

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI_HOST;

  sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_cfg.gpio_cs = SD_PIN_CS;
  slot_cfg.host_id = SD_SPI_HOST;

  esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024,
  };

  ret = esp_vfs_fat_sdspi_mount(SDCARD_MOUNT_POINT, &host, &slot_cfg,
                                &mount_cfg, &s_card);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init sdcard: %s", esp_err_to_name(ret));
    return ret;
  }
  sdmmc_card_print_info(stdout, s_card);

  s_dedup_table = malloc(sizeof(uint64_t) * SDCARD_DEDUP_CAPACITY);
  if (!s_dedup_table) {
    ESP_LOGE(TAG, "Out of memory for dedup table");
    return ESP_ERR_NO_MEM;
  }
  for (int i = 0; i < SDCARD_DEDUP_CAPACITY; i++) {
    s_dedup_table[i] = SDCARD_EMPTY_SLOT;
  }

  snprintf(s_file_path, sizeof(s_file_path), "%s/%s", SDCARD_MOUNT_POINT,
           filename);

  s_lock = xSemaphoreCreateMutex();
  if (!s_lock) {
    return ESP_ERR_NO_MEM;
  }

  struct stat st;
  bool exists = (stat(s_file_path, &st) == 0);
  if (!exists) {
    FILE *f = fopen(s_file_path, "w");
    if (!f) {
      ESP_LOGE(TAG, "Failed to create %s", s_file_path);
      return ESP_FAIL;
    }
    if (header && header[0] != '\0') {
      fputs(header, f);
    }
    fclose(f);
    ESP_LOGI(TAG, "Created file %s", s_file_path);
  } else {
    ESP_LOGI(TAG, "Found existing file %s, loading MACs", s_file_path);
    preload_existing_macs(s_file_path);
  }

  return ESP_OK;
}

bool sdcard_write_row(const uint8_t mac[6], const char *line) {
  if (!s_lock || !s_dedup_table) {
    return false;
  }

  bool written = false;
  xSemaphoreTake(s_lock, portMAX_DELAY);

  uint64_t key = mac_to_key(mac);
  if (dedup_contains(key)) {
    xSemaphoreGive(s_lock);
    return false;
  }

  FILE *f = fopen(s_file_path, "a");
  if (!f) {
    ESP_LOGE(TAG, "Failed to open %s", s_file_path);
    xSemaphoreGive(s_lock);
    return false;
  }
  fputs(line, f);
  fclose(f);

  dedup_insert(key);
  written = true;

  xSemaphoreGive(s_lock);
  return written;
}

void sdcard_deinit(void) {
  if (s_card) {
    esp_vfs_fat_sdcard_unmount(SDCARD_MOUNT_POINT, s_card);
    s_card = NULL;
  }
  spi_bus_free(SD_SPI_HOST);
  free(s_dedup_table);
  s_dedup_table = NULL;
  if (s_lock) {
    vSemaphoreDelete(s_lock);
    s_lock = NULL;
  }
}
