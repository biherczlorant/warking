#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "esp_err.h"
#include "lvgl.h"

lv_disp_t *display_init(void);
bool display_lock(uint32_t timeout_ms);
void display_unlock(void);

#endif // __DISPLAY_H__
