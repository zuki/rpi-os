#include "mm.h"

#include "types.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "spinlock.h"
#include "console.h"
#include "mbox.h"

#ifdef DEBUG

#define MAX_PAGES 1000
static void *alloc_ptr[MAX_PAGES];

#endif

#define KB  (1024UL)
#define MB  (KB * KB)
#define GB  (MB * KB)

extern char end[];

// 実機 free_range: 0xffff000000125000 ~ 0xffff00003b400000, 242395 pages
// QEMU free_range: 0xffff0000000a6000 ~ 0xffff00003c000000, 245594 pages
#define PHYSTOP 0x3b400000
#define NFRAMES (PHYSTOP / PGSIZE)

struct run {
    struct run *next;
};

struct _kmem {
    struct spinlock lock;
    struct run *freelist;
    uint64_t fpages;
} kmem;

struct _kmem_reftable {
    int ref[NFRAMES];
    struct spinlock lock;
} kmem_reftable;

static uint64_t totalram = 0;

static void
free_range(void *start, void *end)
{
    int cnt = 0;

    for (void *p = start; p + PGSIZE <= end; p += PGSIZE, cnt++) {
        kfree(p);
        totalram += PGSIZE;
    }
    info("0x%p ~ 0x%p, %d pages", start, end, cnt);
}

void
mm_init(void)
{
    initlock(&kmem_reftable.lock, "kmem_ref");
    initlock(&kmem.lock, "kmem");

    acquire(&kmem_reftable.lock);
    for (int i = 0; i < NFRAMES; i++) {
        kmem_reftable.ref[i] = 0;
    }
    release(&kmem_reftable.lock);

    kmem.fpages = 0;
    // HACK Raspberry pi 4b.
    //size_t phystop = MIN(0x3F000000, mbox_get_arm_memory());
    free_range(ROUNDUP((void *)end, PGSIZE), P2V(PHYSTOP));
}

void
inc_kmem_ref(uint64_t pa)
{
    acquire(&kmem_reftable.lock);
    kmem_reftable.ref[pa >> PGSHIFT]++;
    release(&kmem_reftable.lock);
}

void
dec_kmem_ref(uint64_t pa)
{
    acquire(&kmem_reftable.lock);
    kmem_reftable.ref[pa >> PGSHIFT]--;
    release(&kmem_reftable.lock);
}

int
get_kmem_ref(uint64_t pa)
{
    acquire(&kmem_reftable.lock);
    int ref = kmem_reftable.ref[pa >> PGSHIFT];
    release(&kmem_reftable.lock);
    return ref;
}

static void
set_kmem_ref(uint64_t pa, int count)
{
    acquire(&kmem_reftable.lock);
    kmem_reftable.ref[pa >> PGSHIFT] = count;
    release(&kmem_reftable.lock);
}

uint64_t
get_totalram(void)
{
    return totalram;
}

uint64_t
get_freeram(void)
{
    acquire(&kmem.lock);
    uint64_t fpages = kmem.fpages;
    release(&kmem.lock);
    return fpages * PGSIZE;
}

/*
 * Allocate a page of physical memory.
 * Returns 0 if failed else a pointer.
 * Corrupt the page by filling non-zero value in it for debugging.
 */
void *
kalloc(void)
{
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if (r) {
        kmem.freelist = r->next;
        kmem.fpages--;
        set_kmem_ref((uint64_t)V2P((uint64_t)r), 1);
    }
    release(&kmem.lock);
    return r;
}

/* Free the physical memory pointed at by v. */
void
kfree(void *va)
{
    struct run *r;
    if ((uint64_t)va % PGSIZE || va < end || V2P(va) >= PHYSTOP)
        panic("kfree: va=0x%p", va);

    acquire(&kmem.lock);
    r = (struct run *)va;
    r->next = kmem.freelist;
    kmem.freelist = r;
    kmem.fpages++;
    set_kmem_ref((uint64_t)V2P((uint64_t)va), 0);
    release(&kmem.lock);
}


void
mm_dump()
{
    cprintf("\nTotal Memory: %lld MB, Free Memmory: %lld MB\n\n", get_totalram() / MB, get_freeram() / MB);
#ifdef DEBUG
    int cnt = 0;
    for (int i = 0; i < MAX_PAGES; i++) {
        if (alloc_ptr[i])
            cnt++;
    }
    debug("allocated: %d pages", cnt);
#endif
}

void
mm_test()
{
#ifdef DEBUG
    static void *p[0x100000000 / PGSIZE];
    int i;
    for (i = 0; (p[i] = kalloc()); i++) {
        memset(p[i], 0xFF, PGSIZE);
        if (i % 10000 == 0)
            debug("0x%p", p[i]);
    }
    while (i--)
        kfree(p[i]);
#endif
}
