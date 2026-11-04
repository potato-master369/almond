// gvt.c
// --------------------------------
// Almond graphical virtual terminal
#include "framebuffer/fb.h"

extern fb_surface_t fb0;
extern const uint8_t terminus_fon[128][16];

static struct {
  uint32_t x;
  uint32_t y;
  bool_t gvt_enabled;
} gvt_context = {.x = 0, .y = 0, .gvt_enabled = false};

static void gvt_print_c(char c) {
  for (uint16_t y = 0; y < 16; ++y) {
    uint8_t row = terminus_fon[c][y];
    for (uint16_t x = 0; x < 8; ++x) {
      fb0.write_pixel(gvt_context.x + x, gvt_context.y + y,
                      (row & (1 << x)) ? 0xAAAAAA00 : 0x00000000, &fb0);
    }
  }
}

static void gvt_newline(void) {
  gvt_context.x = 0;

  if (gvt_context.y + 16 >= fb0.height) {
    fb0.scroll_up(16, 0x00000000, &fb0);
    gvt_context.y = (fb0.height >= 16) ? fb0.height - 16 : 0;
  } else {
    gvt_context.y += 16;
  }
}

void gvt_send_message(char *msg) {
  if (!gvt_context.gvt_enabled)
    return;
  for (int i = 0; msg[i] != '\0'; ++i) {
    if (msg[i] >= 0x20 && msg[i] <= 0x7E) {
      gvt_print_c(msg[i]);
      gvt_context.x += 8;
      if (gvt_context.x + 8 > fb0.width)
        gvt_newline();
    } else if (msg[i] == '\n') {
      gvt_newline();
    }
  }
  fb0.update_fb(&fb0);
}

void gvt_init(void) {
  fb0.clear(0x00000000, &fb0);
  gvt_context.gvt_enabled = true;
  gvt_send_message("Almond Graphical Terminal (Almond Kernel)\nCompile time: " __DATE__ " " __TIME__ "\nSome messages were sent before the framebuffer initialization. They were only reported over COM1.\n================================================================================\n\n");
}
