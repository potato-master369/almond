#include "kernel_types.h"

void *memcpy(void *destination, const void *source, uint32_t count) {
  volatile uint8_t *destination_bytes = (volatile uint8_t *)destination;
  const volatile uint8_t *source_bytes = (const volatile uint8_t *)source;

  for (uint32_t i = 0; i < count; ++i) {
    destination_bytes[i] = source_bytes[i];
  }

  return destination;
}