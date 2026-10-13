// clock.c
// ---------------------
// Handle clock

#include "clock.h"
#include "../kernel_types.h"

// drivers
#include "drv/rtc.h"
#include "drv/pit.h"

almond_ktime_t ctime;

// returns the 64-bit UNIX time of startup
int64_t clock_init(void) {
  int64_t unixtime;
  rtc_read_clock(&unixtime);
  pit_init();
  return unixtime;
}

// close enough
uint32_t clock_get_ms_since_boot(void) {
  return pit_get_ms();
}
