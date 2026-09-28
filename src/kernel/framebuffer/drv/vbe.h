#ifndef VBE_H
#define VBE_H
#include "../../kernel_types.h"
#include "../fb.h"
bool_t vbe_discover(void);
void vbe_put_pixel(uint16_t x, uint16_t y, uint32_t col, struct fb_surface *self);
void vbe_update_fb(struct fb_surface *self);
void vbe_init(fb_surface_t *target);
#endif
