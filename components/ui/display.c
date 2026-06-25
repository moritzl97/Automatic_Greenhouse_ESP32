#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <lvgl.h>
#include <sys/param.h>
#include "display.h"
#include "utils.h"
#include "gpio_config.h"

#define TAG "DISPLAY"

#define LCD_H_RES           240
#define LCD_V_RES           135
#define LVGL_TICK_PERIOD_MS 5

static _lock_t lvgl_api_lock;
static lv_display_t *greenhouse_display = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_timer_handle_t lvgl_tick_timer = NULL;

// lvgl ui handles
static lv_obj_t *display_screen = NULL;
static lv_obj_t *label_description = NULL;
static lv_obj_t *label_value = NULL;

#define MAX_SCREENS 4

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t io_panel,
                                     esp_lcd_panel_io_event_data_t *edata,
                                     void *user_ctx)
{
    lv_display_flush_ready((lv_display_t *)user_ctx);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    esp_lcd_panel_draw_bitmap(panel,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              px_map);
    // flush_ready called in notify_lvgl_flush_ready callback
}

static void increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_task(void *arg)
{
    uint32_t time_till_next = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next = lv_timer_handler();
        _lock_release(&lvgl_api_lock);

        uint32_t delay = MAX(time_till_next, 5);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

void greenhouse_display_init(void)
{
    // backlight on
    // gpio_set_direction(DISPLAY_BL_PIN, GPIO_MODE_OUTPUT);
    // gpio_set_level(DISPLAY_BL_PIN, 1);

    // SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = DISPLAY_MOSI_PIN,
        .miso_io_num     = -1,
        .sclk_io_num     = DISPLAY_CLK_PIN,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // panel IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = DISPLAY_DC_PIN,
        .cs_gpio_num       = DISPLAY_CS_PIN,
        .pclk_hz           = 40000000,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io_handle));

    // ST7789 panel
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = DISPLAY_RST_PIN,
        .rgb_endian     = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 40, 53));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // LVGL
    lv_init();

    greenhouse_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_user_data(greenhouse_display, panel_handle);
    lv_display_set_color_format(greenhouse_display, LV_COLOR_FORMAT_RGB565);

    // double buffered, DMA capable
    size_t buf_size = LCD_H_RES * LCD_V_RES / 10 * 2; // 1/10 screen
    void *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(buf1 && buf2);
    lv_display_set_buffers(greenhouse_display, buf1, buf2, buf_size,
                           LV_DISP_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(greenhouse_display, lvgl_flush_cb);

    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, greenhouse_display);

    // LVGL tick timer
    const esp_timer_create_args_t tick_args = {
        .callback = &increase_lvgl_tick,
        .name     = "lvgl_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    xTaskCreate(lvgl_task, "LVGL", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "ST7789 display initialized %dx%d", LCD_H_RES, LCD_V_RES);
}

static void ui_init_once(void)
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    display_screen = lv_obj_create(NULL);
    lv_screen_load(display_screen);
    lv_obj_set_style_bg_color(display_screen, lv_color_black(), 0);

    // description label (smaller, top)
    label_description = lv_label_create(display_screen);
    lv_obj_set_width(label_description, LCD_H_RES);
    lv_obj_align(label_description, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_align(label_description, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_description, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_description, &lv_font_montserrat_14, 0);

    // value label (larger, center)
    label_value = lv_label_create(display_screen);
    lv_obj_set_width(label_value, LCD_H_RES);
    lv_obj_align(label_value, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_align(label_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_value, lv_color_make(0, 255, 128), 0);
    lv_obj_set_style_text_font(label_value, &lv_font_montserrat_28, 0);
}

void display_draw(measurements_t *measurements, bool button_press)
{
    _lock_acquire(&lvgl_api_lock);
    ui_init_once();

    static int cycle_screen = 0;

    if (button_press) {
        cycle_screen = (cycle_screen + 1) % MAX_SCREENS;
        ESP_LOGI(TAG, "Switching to screen %d", cycle_screen);
    }

    switch (cycle_screen) {
        case 0:
            lv_label_set_text(label_description, "Temperature");
            lv_label_set_text_fmt(label_value, "%d.%d C", (int)measurements->temperature, (int)((measurements->temperature - (int)measurements->temperature) * 10));
            break;
        case 1:
            lv_label_set_text(label_description, "Rel. Humidity");
            lv_label_set_text_fmt(label_value, "%d.%d %%", (int)measurements->relative_humidity, (int)((measurements->relative_humidity - (int)measurements->relative_humidity) * 10));
            break;
        case 2:
            lv_label_set_text(label_description, "Light Intensity");
            lv_label_set_text_fmt(label_value, "%d.%d %%", (int)measurements->light, (int)((measurements->light - (int)measurements->light) * 10));
            break;
        case 3:
            lv_label_set_text(label_description, "Soil Moisture");
            lv_label_set_text_fmt(label_value, "%d.%d %%", (int)measurements->pots[0].soil_moisture, (int)((measurements->pots[0].soil_moisture - (int)measurements->pots[0].soil_moisture) * 10));
            break;
        default:
            break;
    }

    _lock_release(&lvgl_api_lock);
}