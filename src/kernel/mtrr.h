#ifndef MTRR_H
#define MTRR_H

#include "kernel_types.h"


#define MTRR_TYPE_UC 0
#define MTRR_TYPE_WC 1
#define MTRR_TYPE_WT 4
#define MTRR_TYPE_WP 5
#define MTRR_TYPE_WB 6

int mtrr_set_region(uint32_t base, uint32_t size, uint8_t type);
int mtrr_init(void);
void mtrr_enable(bool_t enable);

#endif
