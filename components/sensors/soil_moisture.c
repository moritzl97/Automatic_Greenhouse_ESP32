#include "soil_moisture.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adc_init.h"
#include "gpio_config.h"
#include "utils.h"

#define TAG "SOIL_MOISTURE_SENSOR"

static const adc_channel_t soil_channels[] = {
    SOIL_SENSOR_1_ADC_CHANNEL,
    SOIL_SENSOR_2_ADC_CHANNEL,
    SOIL_SENSOR_3_ADC_CHANNEL,
    SOIL_SENSOR_4_ADC_CHANNEL,
};

static adc_cali_handle_t cali_handle;

static int map_value(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void soil_sensor_init(void)
{
    adc_init_all();
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = SOIL_SENSOR_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);

    ESP_LOGI(TAG, "Soil moisture sensor initialized");
}

void measure_active_soil_moisture(measurements_t *measurement)
{
    for (int i = 0; i < 4; i++) {
        if (!measurement->pots[i].active) continue;

        int raw = 0;
        if (adc_oneshot_read(get_adc1_handle(), soil_channels[i], &raw) == ESP_OK) {
            int percent = map_value(raw, 4095, 1200, 0, 100);
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;
            measurement->pots[i].soil_moisture = percent;
        } else {
            ESP_LOGW(TAG, "Failed to read soil sensor %d", i);
        }
    }
}