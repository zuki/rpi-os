#include "timer.h"

#include "arm.h"
#include "base.h"
#include "irq.h"
#include "console.h"
#include "proc.h"
#include "time.h"

/* Core Timer */
#define CORE_TIMER_CTRL(i)      (LOCAL_BASE + 0x40 + 4*(i))
#define CORE_TIMER_ENABLE       (1 << 1)        /* CNTPNSIRQ */

static uint64_t dt;
static uint64_t cnt;

void
timer_init()
{
    dt = timerfreq() / HZ;       // 10 ms
    asm volatile ("msr cntp_ctl_el0, %[x]"::[x] "r"(1));    // Timer enable
    asm volatile ("msr cntp_tval_el0, %[x]"::[x] "r"(dt));  // Set counter
    put32(CORE_TIMER_CTRL(cpuid()), CORE_TIMER_ENABLE);     // core timer enable
#ifdef USE_GIC
    irq_enable(IRQ_LOCAL_CNTPNS);
    irq_register(IRQ_LOCAL_CNTPNS, timer_intr);
#endif
}

static void
timer_reset()
{
    asm volatile ("msr cntp_tval_el0, %[x]"::[x] "r"(dt));
}

/*
 * This is a per-cpu non-stable version of clock, frequency of
 * which is determined by cpu clock (may be tuned for power saving).
 */
void
timer_intr()
{
    trace("t: %d", ++cnt);
    timer_reset();
    yield();
}
