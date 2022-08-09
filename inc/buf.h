#ifndef INC_BUF_H
#define INC_BUF_H

#include "types.h"
#include "list.h"
#include "sleeplock.h"

#define B_BUSY  0x1     /* Buffer is lockd by some process */
#define B_VALID 0x2     /* Buffer has been read from disk. */
#define B_DIRTY 0x4     /* Buffer needs to be written to disk. */

#define DSIZE   4096

struct buf {
    int flags;
    uint32_t dev;
    uint32_t blockno;
    uint8_t data[DSIZE];

    struct list_head clink; /* LRU cache list. */
    struct list_head dlink; /* Disk buffer list. */
};

void        binit();
void        bwrite(struct buf *b);
void        brelse(struct buf *b);
struct buf *bread(uint32_t dev, uint32_t blockno);

#endif
