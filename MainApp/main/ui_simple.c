/*
 * Simple LVGL UI Implementation
 * Native LVGL API - no GUI Guider dependencies
 */

#include "lvgl.h"
#include "esp_log.h"
#include "ui_simple.h"
#include "adc_bsp.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "UI";

#define BUTTON_GPIO  0  // GPIO0 - Built-in button

// Global UI objects
static lv_obj_t *tabview;
static lv_obj_t *tab_timer;
static lv_obj_t *tab_argument;
static lv_obj_t *tab_wifi;
static lv_obj_t *clock_label;
static lv_obj_t *voltage_label;

// Timer values
static int hour = 11;
static int minute = 25;
static int second = 50;

// Timer callback to update clock
void ui_clock_timer_cb(lv_timer_t *timer)
{
    second++;
    if (second >= 60) {
        second = 0;
        minute++;
    }
    if (minute >= 60) {
        minute = 0;
        hour++;
    }
    if (hour >= 24) {
        hour = 0;
    }
    
    if (clock_label && lv_obj_is_valid(clock_label)) {
        lv_label_set_text_fmt(clock_label, "%02d:%02d:%02d", hour, minute, second);
    }
}

// Timer callback to update voltage
void ui_voltage_timer_cb(lv_timer_t *timer)
{
    float voltage = 0.0f;
    int adc_raw = 0;
    adc_get_value(&voltage, &adc_raw);
    
    if (voltage_label && lv_obj_is_valid(voltage_label)) {
        // Convert to millivolts to avoid float formatting issues in LVGL
        int voltage_mv = (int)(voltage * 1000);
        ESP_LOGI(TAG, "ADC: raw=%d, voltage=%d.%02dV", adc_raw, voltage_mv / 1000, voltage_mv % 1000);
        lv_label_set_text_fmt(voltage_label, "Voltage: %d.%02d V", voltage_mv / 1000, voltage_mv % 1000);
    }
}

// Create Timer Tab
static void create_timer_tab(void)
{
    // Large clock display
    clock_label = lv_label_create(tab_timer);
    lv_label_set_text(clock_label, "11:25:50");
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(clock_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(clock_label);
    
    // Dark background for timer tab
    lv_obj_set_style_bg_color(tab_timer, lv_color_hex(0x292929), 0);
    
    // Start 1 second timer
    lv_timer_create(ui_clock_timer_cb, 1000, NULL);
    
    ESP_LOGI(TAG, "Timer tab created");
}

// Create Argument Tab
static void create_argument_tab(void)
{
    // Get chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    
    // Chip info label
    lv_obj_t *label_chip = lv_label_create(tab_argument);
    lv_label_set_text_fmt(label_chip, "Chip: %s Rev %d", 
                          CONFIG_IDF_TARGET, chip_info.revision);
    lv_obj_set_style_text_font(label_chip, &lv_font_montserrat_16, 0);
    lv_obj_align(label_chip, LV_ALIGN_TOP_LEFT, 10, 10);
    
    // Flash info label
    lv_obj_t *label_flash = lv_label_create(tab_argument);
    lv_label_set_text_fmt(label_flash, "Flash: %luMB", flash_size / (1024 * 1024));
    lv_obj_set_style_text_font(label_flash, &lv_font_montserrat_16, 0);
    lv_obj_align(label_flash, LV_ALIGN_TOP_LEFT, 10, 40);
    
    // CPU cores label
    lv_obj_t *label_cores = lv_label_create(tab_argument);
    lv_label_set_text_fmt(label_cores, "CPU Cores: %d", chip_info.cores);
    lv_obj_set_style_text_font(label_cores, &lv_font_montserrat_16, 0);
    lv_obj_align(label_cores, LV_ALIGN_TOP_LEFT, 10, 70);
    
    // Display resolution
    lv_obj_t *label_res = lv_label_create(tab_argument);
    lv_label_set_text(label_res, "Display: 536x240");
    lv_obj_set_style_text_font(label_res, &lv_font_montserrat_16, 0);
    lv_obj_align(label_res, LV_ALIGN_TOP_LEFT, 10, 100);
    
    // Voltage label (updated by timer)
    voltage_label = lv_label_create(tab_argument);
    lv_label_set_text(voltage_label, "Voltage: --.-V");
    lv_obj_set_style_text_font(voltage_label, &lv_font_montserrat_16, 0);
    lv_obj_align(voltage_label, LV_ALIGN_TOP_LEFT, 10, 130);
    
    // Initialize ADC and start voltage update timer
    adc_bsp_init();
    lv_timer_create(ui_voltage_timer_cb, 2000, NULL);
    
    ESP_LOGI(TAG, "System tab created");
}

// WiFi list button event handler
static void wifi_item_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    const char *ssid = lv_label_get_text(label);
    ESP_LOGI(TAG, "WiFi clicked: %s", ssid);
}

// Scan button event handler
static void scan_btn_event_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "WiFi Scan clicked");
    // TODO: Implement WiFi scan
}

// Create WiFi Tab
static void create_wifi_tab(void)
{
    // WiFi list
    lv_obj_t *list = lv_list_create(tab_wifi);
    lv_obj_set_size(list, 250, 160);
    lv_obj_align(list, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Add WiFi networks (placeholder data)
    const char *networks[] = {"Home WiFi", "Office WiFi", "Guest WiFi", "Mobile Hotspot"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, networks[i]);
        lv_obj_add_event_cb(btn, wifi_item_event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    // Scan button
    lv_obj_t *scan_btn = lv_btn_create(tab_wifi);
    lv_obj_set_size(scan_btn, 100, 50);
    lv_obj_align(scan_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    
    lv_obj_t *scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "Scan");
    lv_obj_center(scan_label);
    
    lv_obj_add_event_cb(scan_btn, scan_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    ESP_LOGI(TAG, "WiFi tab created");
}

// Main UI initialization
void ui_init(void)
{
    // Create tabview with 3 tabs at top
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 40);
    
    // Add tabs
    tab_timer = lv_tabview_add_tab(tabview, "Timer");
    tab_argument = lv_tabview_add_tab(tabview, "System");
    tab_wifi = lv_tabview_add_tab(tabview, "WiFi");
    
    // Create content for each tab
    create_timer_tab();
    create_argument_tab();
    create_wifi_tab();
    
    ESP_LOGI(TAG, "UI initialized successfully");
}

// Get current active tab index
int ui_get_active_tab(void)
{
    if (tabview && lv_obj_is_valid(tabview)) {
        return lv_tabview_get_tab_act(tabview);
    }
    return 0;
}

// Set active tab
void ui_set_active_tab(int index)
{
    if (tabview && lv_obj_is_valid(tabview)) {
        lv_tabview_set_act(tabview, index, LV_ANIM_ON);
    }
}

// Button task to cycle through tabs
static void button_task(void *arg)
{
    uint8_t current_tab = 0;
    
    for (;;) {
        // Check if button is pressed (GPIO0 goes LOW)
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            // Debounce delay
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // Wait for button release
            while (gpio_get_level(BUTTON_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            
            // Cycle to next tab
            current_tab++;
            if (current_tab > 2) {  // We have 3 tabs (0, 1, 2)
                current_tab = 0;
            }
            
            ui_set_active_tab(current_tab);
            ESP_LOGI(TAG, "Switched to tab %d", current_tab);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Initialize GPIO button
void ui_button_init(void)
{
    // Configure GPIO0 as input with pull-up
    gpio_config_t gpio_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    
    ESP_ERROR_CHECK(gpio_config(&gpio_conf));
    
    // Create button monitoring task
    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Button initialized on GPIO%d", BUTTON_GPIO);
}
