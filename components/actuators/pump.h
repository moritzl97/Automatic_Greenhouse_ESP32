#ifndef PUMP_H
#define PUMP_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "status_leds.h"
#include "utils.h"

// Public API
void pump_init(void);
void control_active_pumps(measurements_t *measurement, greenhouse_config_t greenhouse_config);

#endif