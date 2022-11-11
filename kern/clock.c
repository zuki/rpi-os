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
#define LOCAL_TIMER_ROUTE       (LOCAL_BASE + 0x24)
#define ROUTING_BITS(i)         (i)

#define LOCAL_TIMER_CTRL        (LOCAL_BASE + 0x34)
    #define LOCAL_TIMER_CTRL_INTERA      (1 << 29)
    #define LOCAL_TIMER_CTRL_ENABLE      (1 << 28)

#if RASPI <= 3
// ローカルタイマーは10ミリ秒でタイムアウト(jiffies単位)
#define LOCAL_TIMER_RELOAD_VALUE    (38400000 / HZ)      /* 2 * 19.2 MHz = 1 sec, / HZ = 10 ms */
#elif RASPI == 4
#define LOCAL_TIMER_RELOAD_VALUE    (108000000)     /* 2 * 54 MHz */
#endif

#define LOCAL_TIMER_CLR         (LOCAL_BASE + 0x38)
    #define LOCAL_TIMER_CLR_INT     (1 << 31)
    #define LOCAL_TIMER_CLR_RELOAD  (1 << 30)

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
clock_init(void)
{
    put32(LOCAL_TIMER_CTRL, LOCAL_TIMER_CTRL_INTERA | LOCAL_TIMER_CTRL_ENABLE | LOCAL_TIMER_RELOAD_VALUE);
    put32(LOCAL_TIMER_ROUTE, ROUTING_BITS(0));
    put32(LOCAL_TIMER_CLR, LOCAL_TIMER_CLR_RELOAD | LOCAL_TIMER_CLR_INT);
#ifdef USE_GIC
    irq_enable(IRQ_LOCAL_TIMER);
    irq_register(IRQ_LOCAL_TIMER, clock_intr, 0);
#endif
    if (rtc_gettime(&xtime) < 0) {
        xtime.tv_nsec = 0L;
        xtime.tv_sec = 1655644975L;      // 2022/06/21/01:31 UTC
    }
    info("clock init ok");
}

static void
clock_reset()
{
    put32(LOCAL_TIMER_CLR, LOCAL_TIMER_CLR_INT);
}

// 現在時の更新
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
 * リアルタイムクロック（ローカルタイマー）割り込みハンドラ。
 * このクロックはクリスタルクロックなので様々なCPUクロックとは
 * 独立している。（10msごとに呼び出される）
 */
void
clock_intr(void)
{
    ++jiffies;
    //thisproc()->stime = jiffies * 1000000000 / HZ;
    //info("c: %d", jiffies);
    update_times();             // 2. 時計更新
    run_timer_list();           // 3. カーネルタイマー実行
    clock_reset();              // 4. ローカルタイマーリセット
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
            struct proc *p = thisproc();
            ptime = (p->stime + p->utime) * TICK_NSEC;
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
    return jiffies * HZ;
}

long get_ticks(void)
{
    return jiffies;
}
