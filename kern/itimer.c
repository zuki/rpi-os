/*
 * linux/kernel/itimer.c
 *
 * Copyright (C) 1992 Darren Senn
 */

/* These are all the functions necessary to implement itimers */
#include "types.h"
#include "irq.h"
#include "errno.h"
#include "spinlock.h"
#include "proc.h"
#include "linux/time.h"

extern uint64_t jiffies;

/*
 * 明らかなオーバフローを回避するためにtimevalをjiffiesに変更する。
 *
 * tv_*secの値は符号付きだが、itimersを行う際に本当に符号付きの値として
 * 使用すべきについては何も示されていないようだ。POSIXはこれについて言及
 * していない（しかし、alarm()がチェックなしでitimersを使うのであれば、
 * 符号なし演算を使わなければならない)。
 */
static uint64_t
tvtojiffies(struct timeval *value)
{
    uint64_t sec =  (uint64_t) value->tv_sec;
    uint64_t usec = (uint64_t) value->tv_usec;

    if (sec > (ULLONG_MAX / HZ))
        return ULLONG_MAX;
    usec += 1000000 / HZ - 1;
    usec /= 1000000 / HZ;
    return HZ * sec + usec;
}

static void
jiffiestotv(uint64_t jiffies, struct timeval *value)
{
    value->tv_usec = (jiffies % HZ) * (1000000 / HZ);
    value->tv_sec  = jiffies / HZ;
}

long
getitimer(int which, struct itimerval *value)
{
    uint64_t val, interval;
    struct proc *p = thisproc();

    switch (which) {
    case ITIMER_REAL:
        acquire(&p->time_lock);
        interval = p->it_real_incr;
        val = 0;
        if (timer_pending(&p->real_timer)) {
            val = p->real_timer.expires - jiffies;
            if ((int64_t) val <= 0)
                val = 1;
        }
        release(&p->time_lock);
        break;
    case ITIMER_VIRTUAL:
/*
        val = p->it_virt_value;
        interval = p->it_virt_incr;
*/
        return -EFAULT;
        break;
    case ITIMER_PROF:
/*
        val = p->it_prof_value;
        interval = p->it_prof_incr;
*/
        return (-EFAULT);
        break;
    default:
        return (-EINVAL);
    }
    jiffiestotv(val, &value->it_value);
    jiffiestotv(interval, &value->it_interval);
    return 0;
}

void
it_real_fn(uint64_t __data)
{
    struct proc *p = (struct proc *) __data;
    uint64_t interval;

    kill(p->pid, SIGALRM);
    interval = p->it_real_incr;
    if (interval) {
        if (interval > (uint64_t) LLONG_MAX)
            interval = LLONG_MAX;
        p->real_timer.expires = jiffies + interval;
        add_timer(&p->real_timer);
    }
}

long
setitimer(int which, struct itimerval *value, struct itimerval *ovalue)
{
    uint64_t i, j;
    long k;
    struct proc *p = thisproc();

    i = tvtojiffies(&value->it_interval);
    j = tvtojiffies(&value->it_value);
    if (ovalue && (k = getitimer(which, ovalue)) < 0)
        return k;
    switch (which) {
        case ITIMER_REAL:
            del_timer_sync(&p->real_timer);
            p->it_real_value = j;
            p->it_real_incr = i;
            if (!j)
                break;
            if (j > (uint64_t) LONG_MAX)
                j = LONG_MAX;
            i = j + jiffies;
            p->real_timer.expires = i;
            add_timer(&p->real_timer);
            break;
        case ITIMER_VIRTUAL:
            if (j)
                j++;
            p->it_virt_value = j;
            p->it_virt_incr = i;
            break;
        case ITIMER_PROF:
            if (j)
                j++;
            p->it_prof_value = j;
            p->it_prof_incr = i;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}
