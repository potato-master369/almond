#include "kernel_types.h"
#include "clock/rng.h"
#include "messaging/messaging.h"

void sched_tick(void) {
  
}

void sched_init(void) {
  message_send_message(" sched: init\n");
}
