#ifndef K_CLOCK_H
#define K_CLOCK_H
#include "../kernel_types.h"

typedef uint64_t almond_ktime_t; // UNIX time; seconds since epoch of Jan 1, 1970
int64_t clock_init(void);
uint32_t clock_get_ms_since_boot(void);

#endif
