#ifndef CLOCK_PIT_H
#define CLOCK_PIT_H
#include "../../kernel_types.h"
void pit_init(void);
uint32_t pit_get_ms(void);
#endif
