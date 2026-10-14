#include "kernel_types.h"
void idt_init(void);
void idt_register_isr(void (*f)(uint8_t), uint8_t isr_n);
