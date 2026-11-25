// rng.c
// -------------------------
// Helper RANDOM NUMBER GENERATOR
// (gambling)
//
// i love gambling
//
// thats why i play umamusume

#include "kernel_types.h"
#include "messaging/messaging.h"

static uint32_t rng_state = 1;

void rng_seed(uint32_t sihh) { 
  message_send_message(" rng: setting seed\n");
  rng_state = sihh ? sihh : 1; 
}

uint32_t rng_next(void) {
  rng_state = rng_state * 1103515245 + 12345;
  return rng_state;
}

uint32_t rng_randuint(uint32_t min, uint32_t max) {
  if (min > max) {
    uint32_t temp = min;
    min = max;
    max = temp;
  }
  uint32_t range = max - min + 1;
  return min + (rng_next() % range);
}
