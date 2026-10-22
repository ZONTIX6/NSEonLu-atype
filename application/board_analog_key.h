#ifndef BOARD_ANALOG_KEY_H
#define BOARD_ANALOG_KEY_H
#include "oled_input.h"
#include <stdbool.h>
bool BoardAnalogKeyRead(void *context, OLED_button_e *button);
#endif
