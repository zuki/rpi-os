#include <errno.h>
#include "rtc.h"
#include "ds3231.h"
#include "console.h"
#include "time.h"
#include "types.h"


static const unsigned char rtc_days_in_month[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

static inline int is_leap_year(int year)
{
    return (!(year % 4) && (year % 100)) || !(year % 400);
}

static int rtc_month_days(int month, int year)
{
    return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
}

int rtc_valid_tm(struct rtc_time *tm)
{
    if (tm->tm_year > 199
        || ((unsigned)tm->tm_mon) >= 12
        || tm->tm_mday < 1
        || tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year + 1900)
        || ((unsigned)tm->tm_hour) >= 24
        || ((unsigned)tm->tm_min) >= 60
        || ((unsigned)tm->tm_sec) >= 60) {
            cprintf("rtc_valid_tm: invalid\n");
            return -EINVAL;
        }
    return 0;
}

static time_t rtc_tm_to_time(const unsigned int year0, const unsigned int mon0,
        const unsigned int day, const unsigned int hour,
        const unsigned int min, const unsigned int sec)
{
    unsigned int mon = mon0 + 1, year = year0 + 1900;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int) (mon -= 2)) {
        mon += 12;  /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((time_t)
                (year/4 - year/100 + year/400 + 367*mon/12 + day) + year*365 - 719499
            ) * 24 + hour /* now have hours - midnight tomorrow handled here */
        ) * 60 + min /* now have minutes */
    ) * 60 + sec; /* finally seconds */
}

static void rtc_time_to_tm(time_t time, struct rtc_time *tm)
{
    unsigned int month, year;
    unsigned long secs;
    int days;

    /* time must be positive */
    days = time / 86400LL;
    secs = time - (unsigned int) days * 86400;

    /* day of the week, 1970-01-01 was a Thursday */
    tm->tm_wday = (days + 4) % 7;

    year = 1970 + days / 365;
    days -= (year - 1970) * 365
        + LEAPS_THRU_END_OF(year - 1)
        - LEAPS_THRU_END_OF(1970 - 1);
    if (days < 0) {
        year -= 1;
        days += 365 + is_leap_year(year);
    }
    tm->tm_year = year - 1900;
    tm->tm_yday = days + 1;

    for (month = 0; month < 11; month++) {
        int newdays;

        newdays = days - rtc_month_days(month, year);
        if (newdays < 0)
            break;
        days = newdays;
    }
    tm->tm_mon = month;
    tm->tm_mday = days + 1;

    tm->tm_hour = secs / 3600;
    secs -= tm->tm_hour * 3600;
    tm->tm_min = secs / 60;
    tm->tm_sec = secs - tm->tm_min * 60;

    tm->tm_isdst = 0;
}

int rtc_gettime(struct timespec *tp) {
    struct rtc_time t;

    int ret = ds3231_get_datetime(&t);
    if (ret != 0) return ret;

    tp->tv_sec = rtc_tm_to_time(t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    tp->tv_nsec = 0L;

    trace("tp_sec: %lld", tp->tv_sec);

    return 0;

}

int rtc_settime(const struct timespec *tp)
{
    struct rtc_time t;

    rtc_time_to_tm(tp->tv_sec, &t);

    trace("secs=%d, mins=%d, hours=%d, mday=%d, mon=%d, year=%d, wday=%d",
        t.tm_sec, t.tm_min, t.tm_hour, t.tm_mday, t.tm_mon, t.tm_year, t.tm_wday);

    return ds3231_set_datetime(&t);
}

void rtc_init()
{
    ds3231_init();
}
