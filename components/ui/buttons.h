#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include "driver/gpio.h"

// Public functions
void inputs_init(void);
bool get_blue_button_pressed();
bool get_white_button_pressed();
void uart_config_task(void *arg);

#endif
