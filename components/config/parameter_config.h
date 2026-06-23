#ifndef PARAMETER_CONFIG_H
#define PARAMETER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"

extern greenhouse_config_t greenhouse_config;
void reset_to_default_config(void);
void parse_json_to_config(const char *data, int len);

#endif
