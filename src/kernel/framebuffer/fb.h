#ifndef ALMOND_FB_H
#define ALMOND_FB_H
#include "kernel_types.h"

typedef struct fb_surface {
  uint32_t width;
  uint32_t height;
  void (*write_pixel)(uint32_t x, uint32_t y, uint32_t col, struct fb_surface *self);
  void (*blit)(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t *src, uint32_t src_stride, struct fb_surface *self);
  void (*scroll_up)(uint32_t lines, uint32_t fill_col, struct fb_surface *self);
  void (*clear)(uint32_t col, struct fb_surface *self);
  void (*fill_rect)(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col, struct fb_surface *self);
  void (*update_fb)(struct fb_surface *self);
} fb_surface_t;
int fb_init(void);
#endif
