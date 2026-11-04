// fb.c
// --------------------------
// Framebuffer model for Almond
#include "fb.h"
#include "drv/vbe.h"
#include "messaging/messaging.h"

fb_surface_t fb0 = {
  .height = 0,
  .width = 0
};

int fb_init(void) {
  message_send_message(" fb: init");
  if (vbe_discover()) {
    vbe_init(&fb0);
    message_send_message(" fb: selected fb: vbe\n");
    return 0;
  }
  return 1;
}
