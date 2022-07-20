#include "clock.h"
#include "linux/errno.h"
#include "linux/time.h"
#include "arm.h"
#include "base.h"
#include "irq.h"
#include "console.h"
#include "rtc.h"
#include "proc.h"

/* Local timer */
#define TIMER_ROUTE             (LOCAL_BASE + 0x24)
#define TIMER_IRQ2CORE(i)       (i)

#define TIMER_CTRL              (LOCAL_BASE + 0x34)
#define TIMER_INTENA            (1 << 29)
#define TIMER_ENABLE            (1 << 28)

#if RASPI <= 3
#define TIMER_RELOAD_SEC        (38400000 / HZ)      /* 2 * 19.2 MHz = 1 sec, /HZ = 10 ms*/
#elif RASPI == 4
#define TIMER_RELOAD_SEC        (108000000)     /* 2 * 54 MHz */
#endif

#define TIMER_CLR               (LOCAL_BASE + 0x38)
#define TIMER_CLR_INT           (1 << 31)
#define TIMER_RELOAD            (1 << 30)

#define TICK_USEC (10000UL)
#define TICK_NSEC (10000000UL)

uint64_t tick_usec = TICK_USEC;        /* USER_HZ period (usec) */
uint64_t tick_nsec = TICK_NSEC;        /* ACTHZ period (nsec) */

/* The system time (ticks) */
uint64_t jiffies = INITIAL_JIFFIES;
/* The current time (wall_time) */
struct timespec xtime  __attribute__ ((aligned (16)));
/* jiffies at the most recent update of wall time */
unsigned long wall_jiffies = INITIAL_JIFFIES;

void
clock_init()
{
    put32(TIMER_CTRL, TIMER_INTENA | TIMER_ENABLE | TIMER_RELOAD_SEC);
    put32(TIMER_ROUTE, TIMER_IRQ2CORE(0));
    put32(TIMER_CLR, TIMER_RELOAD | TIMER_CLR_INT);
#ifdef USE_GIC
    irq_enable(IRQ_LOCAL_TIMER);
    irq_register(IRQ_LOCAL_TIMER, clock_intr);
#endif
    if (rtc_gettime(&xtime) < 0) {
        xtime.tv_nsec = 0L;
        xtime.tv_sec = 1655644975L;      // 2022/06/21/01:31 UTC
    }
}

static void
clock_reset()
{
    put32(TIMER_CLR, TIMER_CLR_INT);
}

// 現在痔の更新
static void update_wall_time(uint64_t ticks)
{
    do {
        ticks--;
        xtime.tv_nsec += TICK_NSEC;         // 1 tick = 10ms = 10 * 10^6
        if (xtime.tv_nsec >= 1000000000) {  // nsec部分が1秒を超えたら
            xtime.tv_nsec -= 1000000000;    // nsecから1秒引いて
            xtime.tv_sec++;                 // secに1秒足す
        }
    } while (ticks);
}

// 現在時の調整
static inline void update_times(void)
{
    uint64_t ticks;

    ticks = jiffies - wall_jiffies;
    if (ticks) {
        wall_jiffies += ticks;
        update_wall_time(ticks);
    }
}

/*
 * Real time clock (local timer) interrupt. It gets impluse from crystal clock,
 * thus independent of the variant cpu clock.
 */
void
clock_intr()
{
    ++jiffies;
    thisproc()->stime = jiffies * 1000000000 / HZ;
    trace("c: %d", jiffies);
    update_times();
    run_timer_list();
    clock_reset();
}

long
clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    uint64_t ptime;

    switch(clk_id) {
        default:
            return -EINVAL;
        case CLOCK_REALTIME:
            tp->tv_nsec = xtime.tv_nsec;
            tp->tv_sec = xtime.tv_sec;
            break;
        case CLOCK_PROCESS_CPUTIME_ID:
            ptime = thisproc()->stime + thisproc()->utime;
            tp->tv_nsec = ptime % 1000000000;
            tp->tv_sec  = ptime / 1000000000;
            break;
    }
    debug("clk: %d, tv_sec: %lld, tv_nsec: %lld", clk_id, tp->tv_sec, tp->tv_nsec);
    return 0;
}

long clock_settime(clockid_t clk_id, const struct timespec *tp)
{
    switch(clk_id) {
        default:
            return -EINVAL;
        case CLOCK_REALTIME:
            if (capable(CAP_SYS_TIME)) {
                xtime.tv_nsec = tp->tv_nsec;
                xtime.tv_sec = tp->tv_sec;
            } else {
                return -EPERM;
            }
            break;
    }
    return 0;
}

long get_uptime(void)
{
    return xtime.tv_sec;
}
