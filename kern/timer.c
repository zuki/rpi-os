#include "timer.h"
#include "linux/time.h"
#include "arm.h"
#include "base.h"
#include "irq.h"
#include "console.h"
#include "proc.h"
#include "list.h"
#include "spinlock.h"

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


struct spinlock timerlock;

/*
 * Event timer code
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

struct timer_vec {
    int index;
    struct list_head vec[TVN_SIZE];
};

struct timer_vec_root {
    int index;
    struct list_head vec[TVR_SIZE];
};

static struct timer_vec tv5;
static struct timer_vec tv4;
static struct timer_vec tv3;
static struct timer_vec tv2;
static struct timer_vec_root tv1;

static struct timer_vec * const tvecs[] = {
    (struct timer_vec *)&tv1, &tv2, &tv3, &tv4, &tv5
};

#define NOOF_TVECS (sizeof(tvecs) / sizeof(tvecs[0]))

void
init_timervecs(void)
{
    int i;

    for (i = 0; i < TVN_SIZE; i++) {
        list_init(tv5.vec + i);
        list_init(tv4.vec + i);
        list_init(tv3.vec + i);
        list_init(tv2.vec + i);
    }
    for (i = 0; i < TVR_SIZE; i++)
        list_init(tv1.vec + i);

    initlock(&timerlock, "timerlock");
}

static uint64_t timer_jiffies;

static inline void
internal_add_timer(struct timer_list *timer)
{
    /*
     * must be cli-ed when calling this
     */
    uint64_t expires = timer->expires;
    uint64_t idx = expires - timer_jiffies;
    struct list_head *vec;

    if (idx < TVR_SIZE) {
        int i = expires & TVR_MASK;
        vec = tv1.vec + i;
    } else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
        int i = (expires >> TVR_BITS) & TVN_MASK;
        vec = tv2.vec + i;
    } else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
        int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
        vec =  tv3.vec + i;
    } else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
        int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
        vec = tv4.vec + i;
    } else if ((int64_t) idx < 0) {
        /* can happen if you add a timer with expires == jiffies,
         * or you set a timer to go off in the past
         */
        vec = tv1.vec + tv1.index;
    } else if (idx <= 0xffffffffUL) {
        int i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
        vec = tv5.vec + i;
    } else {
        /* Can only get here on architectures with 64-bit jiffies */
        list_init(&timer->list);
        return;
    }
    /*
     * Timers are FIFO!
     */
    list_push_back(vec->prev, &timer->list);
}

void
add_timer(struct timer_list *timer)
{
    acquire(&timerlock);
    if (timer_pending(timer))
        goto bug;
    internal_add_timer(timer);
    release(&timerlock);
    return;
bug:
    release(&timerlock);
    warn("bug: kernel timer added twice at %p.\n",
            __builtin_return_address(0));
}

static inline int
detach_timer(struct timer_list *timer)
{
    if (!timer_pending(timer))
        return 0;
    list_drop(&timer->list);
    return 1;
}

int
mod_timer(struct timer_list *timer, uint64_t expires)
{
    int ret;

    acquire(&timerlock);
    timer->expires = expires;
    ret = detach_timer(timer);
    internal_add_timer(timer);
    release(&timerlock);
    return ret;
}

int
del_timer(struct timer_list * timer)
{
    int ret;

    acquire(&timerlock);
    ret = detach_timer(timer);
    timer->list.next = timer->list.prev = NULL;
    release(&timerlock);
    return ret;
}

static inline void
cascade_timers(struct timer_vec *tv)
{
    /* cascade all the timers from tv up one level */
    struct list_head *head, *curr, *next;

    head = tv->vec + tv->index;
    curr = head->next;
    /*
     * We are removing _all_ timers from the list, so we don't  have to
     * detach them individually, just clear the list afterwards.
     */
    while (curr != head) {
        struct timer_list *tmp;

        tmp = list_entry(curr, struct timer_list, list);
        next = curr->next;
        list_drop(curr); // not needed
        internal_add_timer(tmp);
        curr = next;
    }
    list_init(head);
    tv->index = (tv->index + 1) & TVN_MASK;
}

void
run_timer_list(void)
{
    acquire(&timerlock);
    while ((int64_t)(jiffies - timer_jiffies) >= 0) {
        struct list_head *head, *curr;
        if (!tv1.index) {
            int n = 1;
            do {
                cascade_timers(tvecs[n]);
            } while (tvecs[n]->index == 1 && ++n < NOOF_TVECS);
        }
repeat:
        head = tv1.vec + tv1.index;
        curr = head->next;
        if (curr != head) {
            struct timer_list *timer;
            void (*fn)(uint64_t);
            uint64_t data;

            timer = list_entry(curr, struct timer_list, list);
            fn = timer->function;
            data = timer->data;
            detach_timer(timer);
            timer->list.next = timer->list.prev = NULL;
            //timer_enter(timer);
            release(&timerlock);
            fn(data);
            acquire(&timerlock);
            //timer_exit();
            goto repeat;
        }
        ++timer_jiffies;
        tv1.index = (tv1.index + 1) & TVR_MASK;
    }
    release(&timerlock);
}
