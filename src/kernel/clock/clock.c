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

// busy wait for the given number of milliseconds
void clock_wait_ms(uint32_t ms) {
  uint32_t start = pit_get_ms();
  while ((uint32_t)(pit_get_ms() - start) < ms) {
    __asm__ __volatile__("pause");
  }
}
