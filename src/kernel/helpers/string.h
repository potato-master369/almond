// string.h
// ----------------------
// string things implementations

#ifndef STRING_H
#define STRING_H
#include "../kernel_types.h"
#define NULL (void *)0
static inline int strncmp(const char *s1, const char *s2, int n) {
  while (n-- && *s1 && (*s1 == *s2))
    s1++, s2++;
  return n == (int)-1 ? 0 : *(unsigned char *)s1 - *(unsigned char *)s2;
}
static inline char *itoa(int n, char *s) {
  char *p = (n < 0 ? (*s++ = '-'), s : s);
  return (n / 10 ? p = itoa(n / 10 > 0 ? n / 10 : -(n / 10), p) : p),
         *p++ = '0' + (n < 0 ? -(n % 10) : n % 10), *p = 0, p;
}
static inline void memcpy(void *dst, const void *src, uint32_t n) {
  __builtin_memcpy(dst, src, n);
}

static inline int append_string(char *buf, char *to_append,
                                uint32_t sizeof_buf) {
  if (!buf || !to_append || sizeof_buf == 0) {
    return -1;
  }

  uint32_t cur_len = 0;
  while (buf[cur_len] != '\0' && cur_len < sizeof_buf) {
    cur_len++;
  }
  if (cur_len >= sizeof_buf) {
    return -1; // buf already full / not null-terminated in range
  }

  uint32_t space_left = sizeof_buf - cur_len - 1; // reserve room for null
  uint32_t i = 0;

  while (to_append[i] != '\0' && i < space_left) {
    buf[cur_len + i] = to_append[i];
    i++;
  }

  buf[cur_len + i] = '\0';

  return (to_append[i] == '\0') ? 0 : -1;
}

static inline int append_uint(char *buf, uint32_t to_append,
                              uint32_t sizeof_buf) {
  if (!buf || sizeof_buf == 0) {
    return -1;
  }

  char tmp[12];
  itoa(to_append, tmp);

  uint32_t cur_len = 0;
  while (buf[cur_len] != '\0' && cur_len < sizeof_buf) {
    cur_len++;
  }
  if (cur_len >= sizeof_buf) {
    return -1;
  }

  uint32_t space_left = sizeof_buf - cur_len - 1;
  uint32_t i = 0;

  while (tmp[i] != '\0' && i < space_left) {
    buf[cur_len + i] = tmp[i];
    i++;
  }

  buf[cur_len + i] = '\0';

  return (tmp[i] == '\0') ? 0 : -1;
}

static inline char *strcpy(char *dest, const char *src) {
  if (dest == NULL || src == NULL) {
    return (char *)NULL;
  }

  char *saved_dest = dest;

  while ((*dest++ = *src++) != '\0') {
  }

  return saved_dest;
}

static inline void itoa_hex(uint32_t val, char *numbuf) {
  static const char digits[] = "0123456789abcdef";
  char tmp[8];

  for (int i = 0; i < 8; i++) {
    tmp[i] = digits[(val >> ((7 - i) * 4)) & 0xF];
  }

  int start = 0;
  while (start < 7 && tmp[start] == '0')
    start++;

  int i = 0;
  for (; start < 8; start++, i++) {
    numbuf[i] = tmp[start];
  }
  numbuf[i] = '\0';
}

static inline void utoa64(uint64_t value, char *str) {
    char temp[21];
    int i = 0;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while (value > 0) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    int j = 0;
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}
#endif
