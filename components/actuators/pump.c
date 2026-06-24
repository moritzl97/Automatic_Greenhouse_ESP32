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

#define PUMP_DURATION_MS 1000 // Duration of the pump being on
#define WATERING_PAUSE_S 60

static const int pump_pins[] = {
    PUMP_1_PIN,
    PUMP_2_PIN,
    PUMP_3_PIN,
    PUMP_4_PIN,
};

void pump_init(void) {
    gpio_config_t io_conf_pump1 = {
        .pin_bit_mask = (1ULL << PUMP_1_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_pump1);
    gpio_set_level(PUMP_1_PIN, 1);

    gpio_config_t io_conf_pump2 = {
        .pin_bit_mask = (1ULL << PUMP_2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_pump2);
    gpio_set_level(PUMP_2_PIN, 1);

    gpio_config_t io_conf_pump3 = {
        .pin_bit_mask = (1ULL << PUMP_3_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_pump3);
    gpio_set_level(PUMP_3_PIN, 1);

    gpio_config_t io_conf_pump4 = {
        .pin_bit_mask = (1ULL << PUMP_4_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_pump4);
    gpio_set_level(PUMP_4_PIN, 1);
        
    ESP_LOGI(TAG, "Pump configured");
}

// void pump_control(float moisture, greenhouse_config_t greenhouse_config) {
//     static bool previous_pump_state = false;
//     static int64_t last_watering = 0;
    
//     if (greenhouse_config.pump_override) {
//         // MANUAL override mode
//         //pump_state = greenhouse_config.pump_override_state;
//         //gpio_set_level(PUMP_GPIO, !pump_state);  //inverse due to relay
//         ESP_LOGW(TAG, "Manual override is currently not safe to use");
//         //ESP_LOGI(TAG, "Manual override: %s", pump_state ? "ON" : "OFF");
//     } else {
//         // AUTO mode
//         int64_t now = esp_timer_get_time();
//         if (moisture <= greenhouse_config.pump_soilmoist_threshold_pct && (now - last_watering) >= (WATERING_PAUSE_S * 1000 * 1000)) { 
//             last_watering = esp_timer_get_time();
//             pump_state = true;
//             gpio_set_level(PUMP_1_PIN, !pump_state); //inverse due to relay
//             vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_MS));
//             gpio_set_level(PUMP_1_PIN, pump_state); //inverse due to relay
//             vTaskDelay(pdMS_TO_TICKS(100));
//             ESP_LOGI(TAG, "PUMP ON (Moisture below threshold)");
//         }
//         if (previous_pump_state == true && pump_state == false) {
//             ESP_LOGI(TAG, "PUMP OFF");
//         }
//         previous_pump_state = pump_state;
//     }
// }

static bool previous_pump_state[4];
static int64_t last_watering[4];
static bool pump_state[4];

void control_active_pumps(measurements_t *measurement, greenhouse_config_t greenhouse_config) {
    for (int i = 0; i < 4; i++) {
        if (!measurement->pots[i].active) continue;

        previous_pump_state[i] = false;
        last_watering[i] = 0;
        
        if (greenhouse_config.pump_override) {
            // MANUAL override mode
            //pump_state = greenhouse_config.pump_override_state;
            //gpio_set_level(PUMP_GPIO, !pump_state);  //inverse due to relay
            //ESP_LOGI(TAG, "Manual override: %s", pump_state ? "ON" : "OFF");
            ESP_LOGW(TAG, "Manual override is currently not safe to use");
        } else {
            // AUTO mode
            int64_t now = esp_timer_get_time();
            if (measurement->pots[i].soil_moisture <= greenhouse_config.pump_soilmoist_threshold_pct && (now - last_watering[i]) >= (WATERING_PAUSE_S * 1000 * 1000)) { 
                last_watering[i] = esp_timer_get_time();
                pump_state[i] = true;
                gpio_set_level(pump_pins[i], !pump_state[i]); //inverse due to relay
                vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_MS));
                gpio_set_level(pump_pins[i], pump_state[i]); //inverse due to relay
                vTaskDelay(pdMS_TO_TICKS(100));
                ESP_LOGI(TAG, "PUMP ON (Moisture below threshold)");
            }
            if (previous_pump_state[i] == true && pump_state[i] == false) {
                ESP_LOGI(TAG, "PUMP OFF");
            }
            previous_pump_state[i] = pump_state[i];
        }
    }
}