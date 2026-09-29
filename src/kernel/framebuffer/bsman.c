// bsman.c
// ---------------------
// Bootsplash manager
//
// Controls bootsplash status bar
#include "fb.h"
#include "../kernel_types.h"

extern fb_surface_t fb0;
extern const uint8_t terminus_fon[128][16];
bool_t bsman_active = false;

void bsman_update(const char *msg) {
  if (!bsman_active)
	  return;
  int i = 0;
  for (int y = fb0.height - 16; y < fb0.height; ++y) {
    for (int x = 0; x < fb0.width; ++x) {
      fb0.write_pixel(x, y, 0xFFFFFFFF, &fb0);
    }
  }
  while (msg[i] != '\0') {
    for (uint16_t y = 0; y < 16; ++y) {
      uint8_t row = terminus_fon[msg[i]][y];
      if ((i * 8) + 8 >= fb0.width)
        break;
      for (uint16_t x = 0; x < 8; ++x) {
        fb0.write_pixel((i * 8) + x, fb0.height - 16 + y, (row & (1 << x)) ? 0x00000000 : 0xffffffff, &fb0);
      }
    }
    ++i;
  }
  fb0.update_fb(&fb0);
}

void bsman_init() {
  bsman_active = true;
}


void bsman_exit() {
  bsman_active = false;
}
