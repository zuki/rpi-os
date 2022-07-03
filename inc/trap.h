#ifndef INC_TRAP_H
#define INC_TRAP_H

#include "types.h"

struct trapframe {
    uint64_t spsr, elr, sp, tpidr;
    uint64_t x[31];
    uint64_t padding;
    __uint128_t q0;
};

void trap(struct trapframe *);
void trap_init();

#endif
