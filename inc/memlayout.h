#ifndef INC_MEMLAYOUT_H
#define INC_MEMLAYOUT_H

/* Stack must always be 16 bytes aligned. */
#define KSTACKSIZE 4096
#define USTACKSIZE 4096

// Deprecated: use mbox_get_arm_memory() instead
// #define PHYSTOP 0x3E000000            /* Top physical memory */

#define KERNBASE 0xFFFF000000000000UL   /* First kernel virtual address */
#define KERNLINK (KERNBASE+0x80000UL)   /* Address where kernel is linked */
#define USERTOP  0x0001000000000000UL   /* Top address of user space. */
#define STACKTOP    USERTOP             /* スタックはUSERTOPから */
#define MMAPBASE    0x600000000000UL    /* Base of mmap address */
#define MMAPTOP     USERTOP             /* ARGV, ENVPなどはstack_topからmmapするため */

#define V2P_WO(x) ((x) - KERNBASE)    /* Same as V2P, but without casts */
#define P2V_WO(x) ((x) + KERNBASE)    /* Same as P2V, but without casts */


#ifndef __ASSEMBLER__

#include "types.h"
#define V2P(a) (((uint64_t) (a)) - KERNBASE)
#define P2V(a) ((void *)(((char *) (a)) + KERNBASE))

#endif

#endif
