#ifndef CLOCK_RNG_H
#define CLOCK_RNG_H
#include "kernel_types.h"
void rng_seed(uint32_t sihh);
uint32_t rng_next(void);
uint32_t rng_randuint(uint32_t min, uint32_t max);
#endif
