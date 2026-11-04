#include "kernel_types.h"

size_t strcspn(const char *s1, const char *s2) {
  const char *p;
  const char *scan;

  for (p = s1; *p != '\0'; ++p) {
    for (scan = s2; *scan != '\0'; ++scan) {
      if (*p == *scan) {
        return (size_t)(p - s1);
      }
    }
  }

  return (size_t)(p - s1);
}
