#include "kernel_types.h"

typedef struct {
  uint32_t eax, ebx, ecx, edx, esp, ebp, esi, edi;
  uint16_t ds, es, fs, gs;
} x86_state_t;
