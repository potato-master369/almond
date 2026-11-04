#include "kernel_types.h"
// some wonderful non-portable x86 shit. Surely it wont bite me back in the tail...

// Optimized cos old memcpy slow af
void *memcpy(void *dest, const void *src, size_t n) {
  void *ret = dest;
  size_t dwords = n >> 2;
  size_t rest = n & 3;
  __asm__ volatile("cld; rep movsl"
                   : "+D"(dest), "+S"(src), "+c"(dwords)
                   :
                   : "memory");
  __asm__ volatile("rep movsb" : "+D"(dest), "+S"(src), "+c"(rest) : : "memory");
  return ret;
}


// Additions
void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;

  if (n == 0 || d == s)
    return dest;

  if (d < s || d >= s + n) {
    return memcpy(dest, src, n);
  }

  d += n - 1;
  s += n - 1;
  __asm__ volatile("std; rep movsb; cld"
                   : "+D"(d), "+S"(s), "+c"(n)
                   :
                   : "memory", "cc");
  return dest;
}

void *memset(void *ptr, int value, size_t num) {
  void *ret = ptr;
  uint32_t v = (uint8_t)value;
  v |= v << 8;
  v |= v << 16;
  size_t dwords = num >> 2;
  size_t rest = num & 3;
  __asm__ volatile("cld; rep stosl"
                   : "+D"(ptr), "+c"(dwords)
                   : "a"(v)
                   : "memory");
  __asm__ volatile("rep stosb" : "+D"(ptr), "+c"(rest) : "a"(v) : "memory");
  return ret;
}
