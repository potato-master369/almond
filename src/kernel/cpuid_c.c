// cpuid_c.c
// ------------------------
// CPUID controller for Almond
// Complements cpuid.asm

#include "cpuid_c.h"


void almond_cpuid_req(uint8_t leaf, almond_cpuid_t *ret) {
    if (!ret) return;

    __asm__ volatile (
        "pushl %%ebx\n\t"
        "cpuid\n\t"
        "movl %%ebx, %1\n\t"
        "popl %%ebx"
        : "=a"(ret->eax), "=r"(ret->ebx), "=c"(ret->ecx), "=d"(ret->edx)
        : "a"(leaf), "c"(0)  // clear ecx as standard for main leaf queries
    );
}
