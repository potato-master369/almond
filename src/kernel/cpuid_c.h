#ifndef CPUID_C_H
#define CPUID_C_H
#include "kernel_types.h"
typedef struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
} almond_cpuid_t;
void almond_cpuid_req(uint32_t leaf, uint32_t subleaf, almond_cpuid_t *ret);
#endif
