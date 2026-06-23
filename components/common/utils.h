#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int id;
    char name[20];
    int gpio_pump;
    int gpio_fan;
    bool active;
    float soil_moisture;
} pot;

typedef struct {
    float temperature;
    float relative_humidity;
    float light;
    pot pots[4];
} measurements_t;

typedef struct {
    uint32_t measurement_interval_s;
    float fan_temp_higher_threshold_C;
    float fan_temp_lower_threshold_C;
    float fan_hum_higher_threshold_pct;
    float fan_hum_lower_threshold_pct;
    float pump_soilmoist_threshold_pct;
    float growlight_light_threshold_pct;
    bool growlight_override;      
    bool growlight_override_state; 
    bool pump_override;
    bool pump_override_state;
    bool fan_override;
    bool fan_override_state;
    char wifi_ssid[33];
    char wifi_password[65];
    bool wifi_reconfigure;
    bool config_updated;
} greenhouse_config_t;

#endif