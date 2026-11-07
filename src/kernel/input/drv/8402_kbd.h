#ifndef KBD_8042_H
#define KBD_8042_H
#include "kernel_types.h"
int kbd_8042_init(void);
void kbd_8042_set_light(bool_t scrlock, bool_t numlock, bool_t capslock);
#endif
