#ifndef INC_RANDOM_H
#define INC_RANDOM_H

#include "base.h"
#include "types.h"

#define RNG_BASE        (MMIO_BASE + 0x00104000)

#define RNG_CTRL        (RNG_BASE + 0x00)
#define RNG_STATUS      (RNG_BASE + 0x04)
#define RNG_DATA        (RNG_BASE + 0x08)
#define RNG_INT_MASK    (RNG_BASE + 0x10)

/* enalbe rng */
#define RNG_RBGEN           0x1
/* the initial numbers generated are "less random" so will be discarded */
#define RNG_WARMUP_COUNT    0x40000
/* interrupt off */
#define RNG_INT_OFF         0x1

void rand_init(void);
uint32_t rand(void);
long getrandom(void *, size_t);

#endif
