#ifndef INC_PAGECACHE_H
#define INC_PAGECACHE_H

#include "file.h"
#include "sleeplock.h"
#include "types.h"

#define NPAGECACHE 1024

struct cached_page {
    char *page;
    uint32_t dev;
    uint32_t inum;
    int ref_count;
    off_t offset;
    struct sleeplock lock;
};

void pagecache_init(void);
void update_page(off_t, uint32_t, uint32_t, char *, size_t);
long copy_page(struct inode *, off_t, char *, size_t, off_t);
#endif
