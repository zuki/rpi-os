#ifndef INC_RTC_H
#define INC_RTC_H

#include "linux/time.h"

// 実機用にコンパイルする場合は以下をコメントアウト
#define USING_RASPI
#undef USE_QEMU_USB_FIX

// QEMU用にコンパイルする場合は以下をコメントアウト
//#undef  USING_RASPI
//#define USE_QEMU_USB_FIX


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
