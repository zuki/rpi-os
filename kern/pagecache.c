#include "pagecache.h"
#include "console.h"
#include "errno.h"
#include "mmu.h"
#include "mm.h"
#include "sleeplock.h"
#include "spinlock.h"
#include "string.h"
#include "types.h"
#include "vfs.h"
#include "proc.h"

struct {
    struct cached_page pages[NPAGECACHE];
    int total_count;
    struct spinlock lock;
} pagecache;

void pagecache_init(void)
{
    initlock(&pagecache.lock);

    acquire(&pagecache.lock);
    for (int i = 0; i < NPAGECACHE; i++) {
        pagecache.pages[i].page = kalloc();
        if (!pagecache.pages[i].page) {
            cprintf("pagecache_init: momory exhausted: i=%d\n", i);
            return;
        }
        initsleeplock(&pagecache.pages[i].lock, "pagecache page");
        //memset(pagecache.pages[i].page, 0, PGSIZE);   // get_page時に初期化
    }
    pagecache.total_count = 0;
    release(&pagecache.lock);
    cprintf("pagecache_init ok\n");
}

static struct cached_page *find_page(uint32_t inum, off_t offset, uint32_t dev)
{
    if (!pagecache.lock.locked)
        panic("not locked");
    for (int i = 0; i < NPAGECACHE; i++) {
        if (pagecache.pages[i].inum == inum &&
            pagecache.pages[i].offset == offset &&
            pagecache.pages[i].dev == dev) {
            debug("- found page %d", i);
            return &pagecache.pages[i];
        }
    }
    return 0;
}

static struct cached_page *get_page(struct inode *ip, off_t offset)
{
    offset -= offset % PGSIZE;
    acquire(&pagecache.lock);
    struct cached_page *res = find_page(ip->inum, offset, ip->dev);
    if (res) {
        release(&pagecache.lock);
        acquiresleep(&res->lock);
        return res;
    }

    struct cached_page *cached_page = &pagecache.pages[pagecache.total_count++];
    release(&pagecache.lock);
    acquiresleep(&cached_page->lock);
    if (pagecache.total_count == NPAGECACHE) {
        pagecache.total_count = 0;
    }
    memset(cached_page->page, 0, PGSIZE);
    int n = readi(ip, cached_page->page, offset, PGSIZE);
    if (n < 0) {
        warn("get_page readi failed: n=%d, offset=%ld, size=%d",
            n, offset, PGSIZE);
        return (struct cached_page *)-1;
    }
    cached_page->dev = ip->dev;
    cached_page->inum = ip->inum;
    cached_page->offset = offset;
    debug("alloc new cached page [%d]", pagecache.total_count - 1);
    return cached_page;
}

long copy_page(struct inode *ip, off_t offset, char *dest, size_t size, off_t dest_offset)
{
    trace("copy_page: inum=%d, offset=%d, dest=0x%p, size=0x%x, dest_offset=%ld",
            ip->inum, offset, dest, size, dest_offset);
    struct cached_page *page = get_page(ip, offset);
    if (page == (struct cached_page *)-1) {
        warn("get_page failed");
        return -ENOMEM;
    }
    if (!holdingsleep(&page->lock))
        panic("copy_page: not locked\n");
    debug("copy_page: memove from 0x%p to 0x%p with 0x%x bytes",
        page->page + dest_offset, dest, size);
    memmove(dest, page->page + dest_offset, size);
    releasesleep(&page->lock);
    return 0;
}

void update_page(off_t offset, uint32_t inum, uint32_t dev, char *addr, size_t size)
{
    debug("addr=0x%p, size=0x%x, offset=0x%x", addr, size, offset);
    acquire(&pagecache.lock);
    off_t alligned_offset = offset - (offset % PGSIZE);
    off_t start_addr = offset % PGSIZE;
    debug("  - aligned_offset=0x%d", alligned_offset);
    struct cached_page *res = find_page(inum, alligned_offset, dev);
    release(&pagecache.lock);

    if (!res) return;   // 該当ページなし

    acquiresleep(&res->lock);
    char *page = res->page;
    debug("    - addr=0x%p, page_offset=0x%x, size=0x%x", addr, start_addr, size);
    debug("update_page: memove from 0x%p to 0x%p with 0x%x bytes", addr, page + start_addr, size);
    memmove(page + start_addr, addr, size);
    releasesleep(&res->lock);
}
