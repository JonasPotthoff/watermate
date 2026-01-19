/*
 * Simple LVGL UI for ESP32-S3 Display
 * Clean implementation without GUI Guider bloat
 */

#ifndef UI_SIMPLE_H
#define UI_SIMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Initialize the UI
void ui_init(void);

// Initialize button for tab switching
void ui_button_init(void);

// Timer callback
void ui_clock_timer_cb(lv_timer_t *timer);

// Tab management
int ui_get_active_tab(void);
void ui_set_active_tab(int index);

#ifdef __cplusplus
}
#endif

#endif // UI_SIMPLE_H
