#include "arm.h"
#include "spinlock.h"
#include "console.h"
#include "proc.h"

void
initlock(struct spinlock *lk, char *name)
{
    lk->locked = 0;
    lk->name = name;
}

void
acquire(struct spinlock *lk)
{
    while (lk->locked
           || __atomic_test_and_set(&lk->locked, __ATOMIC_ACQUIRE)) ;
}

void
release(struct spinlock *lk)
{
    if (!lk->locked)
        panic("release: %s not locked\n", lk->name);
    __atomic_clear(&lk->locked, __ATOMIC_RELEASE);
}
