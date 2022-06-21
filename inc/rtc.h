#ifndef INC_RTC_H
#define INC_RTC_H

#include "time.h"
#include "types.h"

//#define USING_RASPI 1
#undef    USING_RASPI

struct rtc_time {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

void rtc_init();
int rtc_valid_tm(struct rtc_time *tm);
int rtc_gettime(struct timespec *xtime);
int rtc_settime(const struct timespec *xtime);

#endif
