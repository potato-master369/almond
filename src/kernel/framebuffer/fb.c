// fb.c
// --------------------------
// Framebuffer model for Almond
#include "fb.h"
#include "drv/vbe.h"
#include "../messaging/messaging.h"

fb_surface_t fb0;

int fb_init(void) {
  message_send_message(" fb: init");
  if (vbe_discover()) {
    vbe_init(&fb0);
    return 0;
  }
  return 1;
}
