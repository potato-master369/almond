#ifndef KEYMAPS_KEYMAP_H
#define KEYMAPS_KEYMAP_H
#include "kernel_types.h"
typedef struct __attribute__((packed)) {
  char magic[4];
  int len;
  struct {
    uint16_t key;
    uint8_t value;
  } data[];
} almond_keymap_t;
#endif
