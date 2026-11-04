// cpuid_c.c
// ------------------------
// CPUID controller for Almond
// Complements cpuid.asm

#include "kernel_types.h"
#include "cpuid_c.h"

void almond_cpuid_req(uint32_t leaf, uint32_t subleaf,
                      almond_cpuid_t *ret)
{
    if (!ret)
        return;

    __asm__ volatile (
        "cpuid"
        : "=a"(ret->eax),
          "=b"(ret->ebx),
          "=c"(ret->ecx),
          "=d"(ret->edx)
        : "a"(leaf),
          "c"(subleaf)
    );
}
