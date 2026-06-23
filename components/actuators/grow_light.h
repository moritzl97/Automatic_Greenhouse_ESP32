#ifndef GROW_LIGHT_H
#define GROW_LIGHT_H

#include "utils.h"

void grow_light_init(void);
void grow_light_control(float light_level, greenhouse_config_t greenhouse_config);

#endif
