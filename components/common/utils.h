#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char name[20];
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
    float pump_soilmoist_threshold_pct;
    float growlight_light_threshold_pct;
    bool growlight_override;      
    bool growlight_override_state; 
    bool pump_override;
    bool pump_override_state;
    char wifi_ssid[33];
    char wifi_password[65];
    bool wifi_reconfigure;
    bool config_updated;
} greenhouse_config_t;

#endif