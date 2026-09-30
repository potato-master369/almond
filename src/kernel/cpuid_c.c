// cpuid_c.c
// ------------------------
// CPUID controller for Almond
// Complements cpuid.asm

#include "cpuid_c.h"
#include <cpuid.h>

#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
    #define USE_CPUID_H 1
    #include <cpuid.h>
#endif

void almond_cpuid_req(uint8_t leaf, almond_cpuid_t *ret) {
    if (!ret) return;

#if defined(USE_CPUID_H)
    if (!__get_cpuid(leaf, &ret->eax, &ret->ebx, &ret->ecx, &ret->edx)) {
        // clear struct if the hardware doesn't support this specific leaf
        ret->eax = ret->ebx = ret->ecx = ret->edx = 0;
    }

#elif defined(__i386__) // if we are on stinky non-GCC compiler
    // Manually preserves ebx to prevent compiler errors under PIC (-fPIC) configurations
    __asm__ volatile (
        "pushl %%ebx\n\t"
        "cpuid\n\t"
        "movl %%ebx, %1\n\t"
        "popl %%ebx"
        : "=a"(ret->eax), "=r"(ret->ebx), "=c"(ret->ecx), "=d"(ret->edx)
        : "a"(leaf), "c"(0)  // clear ecx as standard for main leaf queries
    );

#else
    // what???
    ret->eax = ret->ebx = ret->ecx = ret->edx = 0;
#endif
}
