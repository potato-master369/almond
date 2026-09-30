#ifndef KMALLOC_H
#define KMALLOC_H

#include "../kernel_types.h"

void kmalloc_init(void);
void *kmalloc(uint32_t size);
void kfree(void *ptr);

void *kmalloc_aligned(uint32_t size, uint32_t align);
void kfree_aligned(void *ptr);
uint32_t kmalloc_virt_to_phys(uint32_t virtual_address);
#endif
