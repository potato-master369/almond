// fb.c
// --------------------------
// Framebuffer model for Almond
#include "fb.h"
#include "drv/vbe.h"

fb_surface_t fb0;

void fb_init(void) {
  if (vbe_discover()) {
    vbe_init(&fb0);
    return;
  }
}
