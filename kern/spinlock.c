#include "arm.h"
#include "spinlock.h"
#include "console.h"
#include "proc.h"

static int
holding(struct spinlock *lk)
{
    return lk->locked && lk->cpu == thiscpu;
}

void
initlock(struct spinlock *lk, char *name)
{
    lk->locked = 0;
    lk->name = name;
    lk->cpu = NULL;
}

void
acquire(struct spinlock *lk)
{
    while (lk->locked
           || __atomic_test_and_set(&lk->locked, __ATOMIC_ACQUIRE)) ;
    lk->cpu = thiscpu;
}

void
release(struct spinlock *lk)
{
    if (!holding(lk)) {
        panic("spinlock cpu %d not held '%s' lock\n", cpuid(), lk->name);
    }
    lk->cpu = NULL;
    __atomic_clear(&lk->locked, __ATOMIC_RELEASE);
}
