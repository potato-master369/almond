#ifndef ALMOND_FB_H
#define ALMOND_FB_H
#include "../kernel_types.h"

typedef struct fb_surface {
  uint32_t width;
  uint32_t height;
  void (*write_pixel)(uint16_t x, uint16_t y, uint32_t col, struct fb_surface *self);
  void (*update_fb)(struct fb_surface *self);
} fb_surface_t;
void fb_init(void);
#endif
