// gerr.c
// ------------------------------
// GUI Error!!

#include "framebuffer/fb.h"
#include "kernel_types.h"

extern const uint8_t terminus_fon[128][16];
extern fb_surface_t fb0;

#define GERR_WIN_W 240
#define GERR_WIN_H 160

// inline to draw the window template
static inline void gerr_draw_window(void) {
  for (uint32_t x = (fb0.width - GERR_WIN_W) / 2;
       x < (fb0.width + GERR_WIN_W) / 2; ++x) {
    for (uint32_t y = (fb0.height - GERR_WIN_H) / 2;
         y < (fb0.height + GERR_WIN_H) / 2; ++y) {
      fb0.write_pixel(x, y, 0x008080FF, &fb0);
    }
  }
  // 1px border
  for (uint32_t x = (fb0.width - GERR_WIN_W) / 2;
       x < (fb0.width + GERR_WIN_W) / 2; ++x) {
    fb0.write_pixel(x, (fb0.height - GERR_WIN_H) / 2, 0x000000FF, &fb0);
    fb0.write_pixel(x, (fb0.height + GERR_WIN_H) / 2 - 1, 0x000000FF, &fb0);
  }

  for (uint32_t y = (fb0.height - GERR_WIN_H) / 2;
       y < (fb0.height + GERR_WIN_H) / 2; ++y) {
    fb0.write_pixel((fb0.width - GERR_WIN_W) / 2, y, 0x000000FF, &fb0);
    fb0.write_pixel((fb0.width + GERR_WIN_W) / 2 - 1, y, 0x000000FF, &fb0);
  }

  // 2px separator
  for (uint32_t x = (fb0.width - GERR_WIN_W) / 2 + 1;
       x < (fb0.width + GERR_WIN_W) / 2 - 1; ++x) {
    fb0.write_pixel(x, (fb0.height - GERR_WIN_H) / 2 + 26, 0x000000FF, &fb0);
    fb0.write_pixel(x, (fb0.height - GERR_WIN_H) / 2 + 27, 0x000000FF, &fb0);
  }
}

static inline void gerr_draw_text(int x, int y, int maxx, int maxy,
                                  char *text) {
  int cx = x;
  int cy = y;
  for (int i = 0; text[i] != '\0'; ++i) {
    if (cx + 8 >= maxx) {
      cx = x;
      cy += 16;
      if (cy + 16 >= maxy)
        break;
    }
    for (uint16_t ay = 0; ay < 16; ++ay) {
      uint8_t row = terminus_fon[text[i]][ay];
      for (uint16_t ax = 0; ax < 8; ++ax) {
        fb0.write_pixel(cx + ax, cy + ay,
                        (row & (1 << ax)) ? 0x00000000 : 0x008080FF, &fb0);
      }
    }
    cx += 8;
  }
}

void gerr_draw_err(char *err) {
  if (err[0] == 'E' && err[1] == 'R' && err[2] == 'R' && err[3] == ':') {
    gerr_draw_window();
    gerr_draw_text((fb0.width - GERR_WIN_W) / 2 + 8,
                   (fb0.height - GERR_WIN_H) / 2 + 8,
                   (fb0.width + GERR_WIN_W) / 2 - 8,
                   (fb0.height - GERR_WIN_H) / 2 + 25, "Error");
    gerr_draw_text((fb0.width - GERR_WIN_W) / 2 + 8,
                   (fb0.height - GERR_WIN_H) / 2 + 32,
                   (fb0.width + GERR_WIN_W) / 2 - 8,
                   (fb0.height + GERR_WIN_H) / 2 - 16, err + 4);
    fb0.update_fb(&fb0);
  }
}
