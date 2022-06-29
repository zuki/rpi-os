#include "trap.h"

#include "arm.h"
#include "sysregs.h"
#include "mmu.h"
#include "irq.h"
#include "memlayout.h"
#include "console.h"
#include "proc.h"
#include "types.h"
#include "mmap.h"
#include "vm.h"
#include "linux/mman.h"
#include "debug.h"

extern long syscall1(struct trapframe *);

static long pf_handler(int dfs, uint64_t far);
void trap_error(uint64_t type);

void
trap_init()
{
    extern char vectors[];
    lvbar(vectors);
    lesr(0);
}

void
trap(struct trapframe *tf)
{
    uint64_t esr = resr();
    uint64_t far = rfar();
    int ec  = (int)(esr >> EC_SHIFT);
    int iss = (int)(esr & ISS_MASK);
    int il  = (int)(esr & IR_MASK);
    int dfs = (int)(iss & 0x3f);
    /* Clear esr. */
    lesr(0);
    switch (ec) {
    case EC_UNKNOWN:
        if (il) {
            panic("unknown error");
        } else {
            irq_handler();
            check_pending_signal();
        }
        break;

    case EC_SVC64:
        if (iss == 0) {
            tf->x[0] = syscall1(tf);
            check_pending_signal();
        } else {
            warn("unexpected svc iss 0x%x", iss);
        }
        break;

    case EC_DABORT:     // 0x24 = 36: ユーザモードで発生
    case EC_DABORT2:    // 0x25 = 37: カーネルモードで発生
        if (dfs >= 4 && dfs <= 15) {
            if ((tf->x[0] = pf_handler(dfs, far)) < 0) {
                thisproc()->killed = 1;
                exit(1);
            }
            check_pending_signal();
        } else {
            info("unknown ec: %d, dfs: %d", ec, dfs);
            trap_error(4);
        }
        break;
    default:
        exit(1);
    }
}

void
trap_error(uint64_t type)
{
    debug_reg();
    panic("irq of type %d unimplemented. \n", type);
}

static long
pf_handler(int dfs, uint64_t far)
{
    struct proc *p = thisproc();
    uint64_t *pte;
    struct mmap_region *region;

    far = ROUNDDOWN(far, PGSIZE);

    if (dfs <= 7) {             // Translation fault
        if ((pte = pgdir_walk(p->pgdir, (void *)far, 1)) == 0)
            return -1;
        return 0;
    } else if (dfs <= 11) {     // Access fault: 遅延読み込み
        region = p->regions;
        while (region) {
            if (region->addr == (void *)far) {
                if (mmap_load_pages(region->addr, region->length, region->prot, region->flags, region->f, region->offset) < 0) {
                    warn("load_pages failed: dfs=%d, far=0x%llx, region=0x%p", dfs, far, region->addr);
                    return -1;
                }
                lttbr0((uint64_t)p->pgdir);
                return 0;
            }
            region = region->next;
        }
        return -1;
    } else {                    // Permission fault: Copy on Write
        region = p->regions;
        while (region) {
            if ((uint64_t)region->addr <= far
             && far < (uint64_t)region->addr + region->length
             && region->flags & MAP_PRIVATE) {
                if (region->prot & PROT_WRITE) {
                    uint64_t perm = get_perm(region->prot, region->flags);
                    perm &= ~PTE_RO;
                    if (copy_mmap_pages(region->addr, region->length, perm) < 0) {
                        warn("copy_mmap_pages failed: dfs=%d, far=0x%llx, region=0x%p, perm=0x%llx", dfs, far, region->addr, perm);
                        return -1;
                    }
                    lttbr0((uint64_t)p->pgdir);
                    return 0;
                }
            }
            region = region->next;
        }
        return -1;
    }
}
