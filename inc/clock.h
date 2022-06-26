#ifndef INC_CLOCK_H
#define INC_CLOCK_H

#include "types.h"
#include "linux/time.h"

void clock_init();
void clock_intr();

long clock_gettime(clockid_t clk_id, struct timespec *tp);
long clock_settime(clockid_t clk_id, const struct timespec *tp);

long get_uptime(void);

#endif
