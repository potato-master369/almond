// handoff_struct.h
// ---------------------------------------
// MIT License; Copyright (C) potato-master369 2026-
// handoff struct for almond kernel.

#ifndef HANDOFF_STRUCT
#define HANDOFF_STRUCT
#include "bl_types.h"

typedef struct __attribute__((packed)) {
  char magic[6]; // almond
  // VBE stuff
  uint32_t vbe_framebuffer;
  uint16_t vbe_pitch;
  uint16_t vbe_width;
  uint16_t vbe_height;
  uint8_t vbe_bpp;
  uint8_t vbe_red_mask;
  uint8_t vbe_red_position;
  uint8_t vbe_green_mask;
  uint8_t vbe_green_position;
  uint8_t vbe_blue_mask;
  uint8_t vbe_blue_position;
  char cmdline[256];
  void *mmap_ptr; // pointer to mmap provided by 0xE820 int 15h
  uint16_t mmap_count;
} almond_handoff_t;
#endif
