#ifndef PIC_H
#define PIC_H
#include "../kernel_types.h"
void pic_send_eoi(uint8_t irq);
void pic_remap(int offset_1, int offset_2);
void pic_disable(void);
#endif
