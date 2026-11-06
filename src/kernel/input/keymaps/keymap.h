#ifndef KEYMAPS_KEYMAP_H
#define KEYMAPS_KEYMAP_H
#include "kernel_types.h"

// DOC BEGIN
//
// Basically how [MODIFIERS] field works is basically 8 bits
//
// struct __attribute__((packed)) _almond_modifiers {
//   bool_t capslock: 1;
//   bool_t shift: 1;
//   bool_t alt: 1;
//   void padding: 5;
// };
//
// DOC END
typedef struct __attribute__((packed)) {
  char magic[4];
  int len;
  struct {
    uint16_t key; // L-E: 0x[MODIFIERS][HID-KEYCODE]
    uint8_t value;
  } data[];
} almond_keymap_t;

void keymap_init(void);
char keymap_convert_to_ascii(uint8_t hid, bool_t caps, bool_t shift, bool_t altgr);
#endif
