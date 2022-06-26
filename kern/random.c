#include "arm.h"
#include "console.h"
#include "random.h"

void
rand_init(void)
{
    // mask interrupt
    uint32_t mask = get32(RNG_INT_MASK);
    mask |= RNG_INT_OFF;
    put32(RNG_INT_MASK, mask);
    // set warm-up count
    put32(RNG_STATUS, RNG_WARMUP_COUNT);
    // enable
    put32(RNG_CTRL, RNG_RBGEN);
    info("rand_init ok");
}

uint32_t
rand(void)
{
    while (!(get32(RNG_STATUS) >> 24)) nop();
    return get32(RNG_DATA);
}

long
getrandom(void *buf, size_t buflen)
{
    uint32_t max_words = (buflen + 3 ) / sizeof(uint32_t);
    uint32_t num_words, count;

    while (!(get32(RNG_STATUS) >> 24)) nop();
    num_words = get32(RNG_STATUS) >> 24;
    if (num_words > max_words)
        num_words = max_words;

    for (count = 0; count < num_words; count++)
        ((uint32_t *)buf)[count] = get32(RNG_DATA);

    return num_words * sizeof(uint32_t);
}
