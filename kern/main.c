#include "types.h"
#include "string.h"
#include "arm.h"
#include "console.h"
#include "vm.h"
#include "mm.h"
#include "clock.h"
#include "timer.h"
#include "trap.h"
#include "proc.h"
#include "emmc.h"
#include "buf.h"
#include "mbox.h"
#include "irq.h"
#include "ds3231.h"
#include "i2c.h"
#include "random.h"

/*
 * Keep it in data segment by explicitly initializing by zero,
 * since we have `-fno-zero-initialized-in-bss` in Makefile.
 */
static struct {
    int cnt;
    struct spinlock lock;
} mp = { 0 };

void
main()
{
    extern char edata[], end[];
    acquire(&mp.lock);
    if (mp.cnt++ == 0) {
        memset(edata, 0, end - edata);
        i2c_init(DS3231_I2C_DIV);
        irq_init();
        console_init();
        mm_init();
        clock_init();
        rand_init();
        pagecache_init();
        proc_init();
        user_init();
        binit();

        // Tests
        mbox_test();
        mm_test();
        vm_test();
    }
    release(&mp.lock);

    timer_init();
    trap_init();
    info("cpu %d init finished", cpuid());

    scheduler();

    panic("scheduler return.\n");
}
