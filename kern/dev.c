#include "list.h"
#include "arm.h"
#include "string.h"

#include "irq.h"

#include "emmc.h"
#include "buf.h"
#include "proc.h"
#include "console.h"
#include "dev.h"
#include "vfs.h"
#include "v6.h"

static void dev_test();

static struct emmc card;
static struct list_head devque;
static struct spinlock cardlock;

// Hack the partition.
static uint32_t first_bno = 0;
static uint32_t nblocks = 1;

// SDカードのMaster Boot Record
struct mbr mbr;
// 使用中のデバイス数
int ndev = 0;

static void
dev_sleep(void *chan)
{
    sleep(chan, &cardlock);
}

/*
 * Initialize SD card and parse MBR.
 * 1. The first partition should be FAT and is used for booting.
 * 2. The second partition is used by our file system.
 *
 * See https://en.wikipedia.org/wiki/Master_boot_record
 */
void
dev_init()
{
    list_init(&devque);
    initlock(&cardlock, "dev");

#if RASPI == 3
    irq_enable(IRQ_SDIO);
    irq_register(IRQ_SDIO, dev_intr);
#elif RASPI == 4
#endif

    acquire(&cardlock);
    int ret = emmc_init(&card, dev_sleep, (void *)&card);
    release(&cardlock);
    assert(ret == 0);

    struct buf b;
    b.blockno = (uint32_t)-1;
    b.flags = 0;
    b.dev = ROOTDEV;
    devrw(&b);
    memmove(&mbr, b.data, 512);
    assert(mbr.signature == 0xAA55);
    for (int i = 0; i < 4; i++) {
        if (mbr.ptables[i].lba == 0) break;
        info("partition[%d]: LBA=0x%x, #SECS=0x%x",
            i, mbr.ptables[i].lba, mbr.ptables[i].nsecs);
        ndev++;
    }

    dev_test();
}

void
dev_intr()
{
    acquire(&cardlock);
    emmc_intr(&card);
    disb();
    wakeup(&card);
    release(&cardlock);
}

/*
 * Start all request.
 * Caller must hold sdlock.
 */
void
dev_start(void)
{
    uint32_t bno;

    while (!list_empty(&devque)) {
        struct buf *b =
            container_of(list_front(&devque), struct buf, dlink);
        if (b->blockno == (uint32_t)-1) {
            bno = 0;
        } else {
            first_bno = sb[b->dev].lba;
            nblocks = sb[b->dev].nsecs;
            assert(b->blockno < nblocks);
            bno = b->blockno * 8 + first_bno;
        }
        emmc_seek(&card, bno * SD_BLOCK_SIZE);
        if (b->flags & B_DIRTY) {
            assert(emmc_write(&card, b->data, BSIZE) == BSIZE);
        } else {
            assert(emmc_read(&card, b->data, BSIZE) == BSIZE);
        }

        b->flags |= B_VALID;
        b->flags &= ~B_DIRTY;

        list_pop_front(&devque);

        disb();
        wakeup(b);
    }
}

void
devrw(struct buf *b)
{
    acquire(&cardlock);

    /* Append to request queue. */
    list_push_back(&devque, &b->dlink);

    /* Start disk if necessary. */
    if (list_front(&devque) == &b->dlink) {
        dev_start();
    }

    /* Wait for request to finish. */
    while ((b->flags & (B_VALID | B_DIRTY)) != B_VALID)
        dev_sleep(b);

    release(&cardlock);
}

/* Test SD card read/write speed. */
static void
dev_test()
{
#ifdef DEBUG
    static struct buf b[1 << 11];
    int n = ARRAY_SIZE(b);
    int mb = (n * BSIZE) >> 20;
    // assert(mb);
    int64_t f, t;
    asm volatile ("mrs %[freq], cntfrq_el0":[freq] "=r"(f));
    info("nblocks %d", n);

    info("check rw...");
    // Read/write test
    for (int i = 1; i < n; i++) {
        // Backup.
        b[0].flags = 0;
        b[0].blockno = i;
        devrw(&b[0]);

        // Write some value.
        b[i].flags = B_DIRTY;
        b[i].blockno = i;
        for (int j = 0; j < BSIZE; j++)
            b[i].data[j] = i * j & 0xFF;
        devrw(&b[i]);

        memset(b[i].data, 0, sizeof(b[i].data));
        // Read back and check
        b[i].flags = 0;
        devrw(&b[i]);
        for (int j = 0; j < BSIZE; j++)
            assert(b[i].data[j] == (i * j & 0xFF));
        // Restore previous value.
        b[0].flags = B_DIRTY;
        devrw(&b[0]);
    }

    // Read profile
    disb();
    t = timestamp();
    disb();
    for (int i = 0; i < n; i++) {
        b[i].flags = 0;
        b[i].blockno = i;
        devrw(&b[i]);
    }
    disb();
    t = timestamp() - t;
    disb();
    info("read %lldB (%lldMB), t: %lld cycles, speed: %lld.%lld MB/s",
         n * BSIZE, mb, t, mb * f / t, (mb * f * 10 / t) % 10);

    // Write profile
    disb();
    t = timestamp();
    disb();
    for (int i = 0; i < n; i++) {
        b[i].flags = B_DIRTY;
        b[i].blockno = i;
        devrw(&b[i]);
    }
    disb();
    t = timestamp() - t;
    disb();

    info("write %lldB (%lldMB), t: %lld cycles, speed: %lld.%lld MB/s\n",
         n * BSIZE, mb, t, mb * f / t, (mb * f * 10 / t) % 10);
#endif
}
