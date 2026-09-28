// messaging.c
// -----------------------------
// kernel messaging fallback
#include "../kernel_types.h"
// drivers
#include "drv/com.h"
#include "../framebuffer/bsman.h"

void message_send_message(const char *message);
void message_init(void) {
  // init drivers
  com_set_baud(); // sets to 38.4K baud cos the default 8250 emulation is slow af
  message_send_message(" message: service begun\n");
}

void message_send_message(const char *message) {
  int i = 0;
  bsman_update(message);
  while (message[i] != '\0') {
          com_send_char(message[i]);
      	  ++i;
  }
}
