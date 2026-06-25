//API key value pairs for MQTT/UART
// "measurement_interval_s":int_value //sets time in seconds between each measurement cycle
// "fan_temp_lower_threshold_C":float_value //set threshold when the fan should turn off again in C
// "fan_temp_higher_threshold_C":float_value //set threshold when the fan should turn on in C
// "fan_hum_lower_threshold_pct":float_value //set threshold when the fan should turn off again in %
// "fan_hum_higher_threshold_pct":float_value //set threshold when the fan should turn on in %
// "pump_soilmoist_threshold_pct":float_value //set threshold when the pump should turn on in %
// "growlight_light_threshold_pct":float_value //set threshold when the growlight should turn on %
// "fan_override":bool_value //set the fan to manual mode (true) or automatic (false)
// "fan":bool_value //set the fan to manual mode and to on or off
// "pump_override":bool_value //set the pump to manual mode (true) or automatic (false)
// "pump":bool_value //set the pump to manual mode and to on or off
// "growlight_override":bool_value //set the grow light to manual mode (true) or automatic (false)
// "growlight":bool_value //set the growlight to manual mode and to on or off
// "config":"default" // resets whole config to default values
// "wifi_ssid":char_string //sets wifi name
// "wifi_password":char_string // sets wifi password
// if one of these values is updated a measurement is executed imidiately

#include <stdio.h>
#include "esp_log.h"
#include "pump.h"
#include "grow_light.h"
#include "utils.h"
#include "display.h"
#include "temp_hum_sensor.h"
#include "light_sensor.h"
#include "buttons.h"
#include "soil_moisture.h"
#include "uart_connection.h"
#include "status_leds.h"
#include "gpio_config.h"
#include "wifi.h"
#include "mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "parameter_config.h"
#include "http_server.h"
#include "esp_http_server.h"
#include "firebase_db.h"

#define TAG "AUTOMATED_GREENHOUSE"
#define DISPLAY_INTERVAL_MS 50

static uint32_t last_measurement_time = 0;
static uint32_t last_display_time = 0;

void take_measurement(measurements_t *measurment)
{
    measurment->light = ldr_read_percent();
    aht20_read(&measurment->temperature, &measurment->relative_humidity);
    measure_active_soil_moisture(measurment);

    ESP_LOGI(TAG, "Measurement taken: temperature: %.2f C, relative humidity: %.2f %%, light intensity: %.2f %%", measurment->temperature, measurment->relative_humidity, measurment->light);
    for (int i = 0; i < 4; i++) {
        if (!measurment->pots[i].active) continue;
        ESP_LOGI(TAG, "Measurement taken: soil moisture %d: %.2f %%", i, measurment->pots[i].soil_moisture);
    }
}

static bool any_pot_needs_water(measurements_t *m)
{
    for (int i = 0; i < 4; i++) {
        if (!m->pots[i].active) continue;
        if (m->pots[i].soil_moisture <= greenhouse_config.pump_soilmoist_threshold_pct) {
            return true;
        }
    }
    return false;
}

void app_main(void)
{   
    // init config of parameters
    reset_to_default_config();
    //  Initialize sensors
    ldr_init();
    soil_sensor_init();
    aht20_init();
    // Initialize actuators
    pump_init();
    grow_light_init();
    // Initialize user Interface
    greenhouse_display_init();

    buttons_init();
    status_leds_init();

    // Initialize Non volatile storage (to store credentials etc)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // WiFi init
    esp_err_t ret1 = wifi_init_sta(greenhouse_config.wifi_ssid, greenhouse_config.wifi_password);
    if (ret1 == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connected successfully");
    } else {
        ESP_LOGE(TAG, "WiFi connection FAILED (error: %s)", esp_err_to_name(ret));
        set_red_connection_led(LED_ON);
    }

    ESP_LOGI(TAG, "Initializing of all components completed");

    measurements_t current_measurements = {
        .temperature = 0,
        .relative_humidity = 0,
        .light = 0,
        .pots = {
            {.name = "plant0", .soil_moisture = 0, .active=true},
            {.name = "plant1", .soil_moisture = 0, .active=true},
            {.name = "plant2", .soil_moisture = 0, .active=true},
            {.name = "plant3", .soil_moisture = 0, .active=true},
        }
    };

    // server init
    ESP_LOGI(TAG, "Starting http server");
    http_server_start(&greenhouse_config, &current_measurements);

    //init uart task
    // xTaskCreate(uart_config_task, "uart_config", 4096, NULL, 5, NULL);
    
    //stop mqtt libraries to clutter log output
    esp_log_level_set("esp-tls", ESP_LOG_NONE);
    esp_log_level_set("mqtt_client", ESP_LOG_NONE);
    esp_log_level_set("transport_base", ESP_LOG_NONE);

    while (1)
    {
        uint32_t now = esp_log_timestamp();

        if (now - last_measurement_time >= greenhouse_config.measurement_interval_s*1000 || greenhouse_config.config_updated)
        {
            greenhouse_config.config_updated = false;
            // sensor measurements
            take_measurement(&current_measurements);

            // connect to new wifi credentials if given
            if (greenhouse_config.wifi_reconfigure) {
                wifi_reconfigure(greenhouse_config.wifi_ssid, greenhouse_config.wifi_password);
                greenhouse_config.wifi_reconfigure = false;
            }
            // Connect to wifi if connection was lost
            if(!wifi_is_connected()) {
                set_red_connection_led(LED_ON);
                ESP_LOGI(TAG, "Trying reconect to wifi");
                wifi_reconnect();
            }
           
            // actuators
            grow_light_control(current_measurements.light, greenhouse_config);

            // pump control
            control_active_pumps(&current_measurements, greenhouse_config);

            last_measurement_time = now;
        }

        static int32_t last_firebase_post = 0;
        if (now - last_firebase_post >= (5 * 60 * 1000 * 1000LL)) {
            firebase_post_measurements(&current_measurements);
            last_firebase_post = now;
        }

        // outputs
        if (now - last_display_time >= DISPLAY_INTERVAL_MS)
        {
            if (get_blue_button_pressed())
            {
                ESP_LOGI(TAG, "Taking extra measurement");
                take_measurement(&current_measurements);
            }

            // leds
            if (any_pot_needs_water(&current_measurements)) {
                set_green_moisture_led(LED_ON);
            } else {
                set_green_moisture_led(LED_OFF);
            }

            //draw display
            display_draw(&current_measurements, get_white_button_pressed());   
            last_display_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
