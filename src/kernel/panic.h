#ifndef PANIC_H
#define PANIC_H
#include "kernel_types.h"
void panic_print(uint32_t err, const char *pretty_name, const char *file, int line);
#define panic_printa(a, b) panic_print(a, b, __FILE__, __LINE__);
#endif
