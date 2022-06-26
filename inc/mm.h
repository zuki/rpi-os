#ifndef INC_MM_H
#define INC_MM_H

#include "types.h"

void mm_init();
void *kalloc();
void kfree(void *v);
void mm_test();
void mm_dump();
uint64_t  get_totalram();
uint64_t  get_freeram();

#endif
