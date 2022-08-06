#ifndef INC_DEV_H
#define INC_DEV_H

#include "buf.h"
#include "types.h"

void dev_init();
void dev_intr();
void devrw(struct buf *);

/* assumes size > 256 */
static inline uint8_t blksize_bits(uint32_t size)
{
  uint8_t bits = 8;
  do {
      bits++;
      size >>= 1;
    } while (size > 256);
  return bits;
}

#endif
