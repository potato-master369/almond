#ifndef INPUT_KEYBOARD_H
#define INPUT_KEYBOARD_H
#include "kernel_types.h"
void keyboard_queue_keystroke(uint16_t keycode, uint8_t pressed);

int keyboard_init(void);
#endif
