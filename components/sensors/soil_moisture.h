#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H

#include "utils.h"

void soil_sensor_init(void);
void measure_active_soil_moisture(measurements_t *measurement);

#endif