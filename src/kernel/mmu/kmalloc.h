#ifndef KMALLOC_H
#define KMALLOC_H

#include "../kernel_types.h"

void kmalloc_init(void);
void *kmalloc(uint32_t size);
void kfree(void *ptr);

#endif