#include "pump.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "buttons.h"
#include "esp_timer.h"
#include "driver/gptimer.h"
#include "utils.h"
#include "gpio_config.h"

#define TAG "PUMP_CONTROL"

#define PUMP_DURATION_MS    500
#define WATERING_COOLDOWN_S 3600   // 1 hour

static const int pump_pins[] = {
    PUMP_1_PIN,
    PUMP_2_PIN,
    PUMP_3_PIN,
    PUMP_4_PIN,
};

void pump_init(void)
{
    for (int i = 0; i < 4; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask  = (1ULL << pump_pins[i]),
            .mode          = GPIO_MODE_OUTPUT,
            .pull_up_en    = GPIO_PULLUP_DISABLE,
            .pull_down_en  = GPIO_PULLDOWN_DISABLE,
            .intr_type     = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(pump_pins[i], 1);  // relay OFF on init (inverted)
    }
    ESP_LOGI(TAG, "Pumps initialized");
}

// static so they persist between calls
static int64_t last_watering[4] = {0, 0, 0, 0};
static bool pump_state[4]       = {false, false, false, false};

void control_active_pumps(measurements_t *measurement, greenhouse_config_t greenhouse_config)
{
    for (int i = 0; i < 4; i++) {
        if (!measurement->pots[i].active) continue;

        if (greenhouse_config.pump_override) {
            // MANUAL override mode
            //pump_state = greenhouse_config.pump_override_state;
            //gpio_set_level(PUMP_GPIO, !pump_state);  //inverse due to relay
            //ESP_LOGI(TAG, "Manual override: %s", pump_state ? "ON" : "OFF");
            ESP_LOGW(TAG, "Manual override is currently not safe to use");
            continue;
        }

        int64_t now = esp_timer_get_time();
        int64_t cooldown_us = (int64_t)WATERING_COOLDOWN_S * 1000 * 1000;
        bool cooldown_elapsed = (now - last_watering[i]) >= cooldown_us;
        bool needs_water = measurement->pots[i].soil_moisture <= greenhouse_config.pump_soilmoist_threshold_pct;

        if (needs_water && cooldown_elapsed) {
            ESP_LOGI(TAG, "Pot %d: moisture %.1f%% below threshold, activating pump", i, measurement->pots[i].soil_moisture);

            last_watering[i] = now;
            pump_state[i] = true;

            gpio_set_level(pump_pins[i], 0);              // relay ON (inverted)
            vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_MS));
            gpio_set_level(pump_pins[i], 1);              // relay OFF (inverted)

            pump_state[i] = false;
            ESP_LOGI(TAG, "Pot %d: pump OFF, cooldown starts (%ds)", i, WATERING_COOLDOWN_S);

        } else if (needs_water && !cooldown_elapsed) {
            int64_t remaining_s = (cooldown_us - (now - last_watering[i])) / 1000 / 1000;
            ESP_LOGI(TAG, "Pot %d: needs water but cooldown active (%llds remaining)",
                     i, remaining_s);
        }
    }
}