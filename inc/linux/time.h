#ifndef INC_LINUX_TIME_H
#define INC_LINUX_TIME_H

#include "types.h"
#include "list.h"

#define HZ  100
//#define INITIAL_JIFFIES ((uint64_t)-300 * HZ)                     # itimerが動かず
//#define INITIAL_JIFFIES (((uint64_t)(-300 * HZ)) & 0xffffffff)    # clock割り込みしない
#define INITIAL_JIFFIES 0UL

struct timeval {
    time_t      tv_sec;
    suseconds_t tv_usec;
};

struct timespec {
    time_t  tv_sec;
    long    tv_nsec;
};

struct itimerval {
        struct timeval it_interval;
        struct timeval it_value;
};

struct timer_list {
    struct list_head list;
    uint64_t expires;
    uint64_t data;
    void (*function)(uint64_t);
};

extern uint64_t jiffies;
extern struct timespec xtime;

#define CURRENT_TIME (xtime.tv_sec)

#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3
#define CLOCK_MONOTONIC_RAW      4
#define CLOCK_REALTIME_COARSE    5
#define CLOCK_MONOTONIC_COARSE   6
#define CLOCK_BOOTTIME           7
#define CLOCK_REALTIME_ALARM     8
#define CLOCK_BOOTTIME_ALARM     9
#define CLOCK_SGI_CYCLE         10
#define CLOCK_TAI               11

#define ITIMER_REAL             0
#define ITIMER_VIRTUAL          1
#define ITIMER_PROF             2

#define TIME_UTC                1


extern void add_timer(struct timer_list *timer);
extern int del_timer(struct timer_list *timer);

#define del_timer_sync(t)   del_timer(t)
#define sync_timers()       do { } while (0)

int mod_timer(struct timer_list *timer, uint64_t expires);

static inline void init_timer(struct timer_list * timer)
{
	timer->list.next = timer->list.prev = NULL;
}

static inline int timer_pending (const struct timer_list * timer)
{
	return timer->list.next != NULL;
}

#define time_after(a,b)     ((int64_t)(b) - (int64_t)(a) < 0)
#define time_before(a,b)    time_after(b,a)

#define time_after_eq(a,b)  ((int64_t)(a) - (int64_t)(b) >= 0)
#define time_before_eq(a,b) time_after_eq(b,a)

void init_timervecs(void);
void run_timer_list(void);
long getitimer(int, struct itimerval *);
void it_real_fn(uint64_t);
long setitimer(int, struct itimerval *, struct itimerval *);

#endif
