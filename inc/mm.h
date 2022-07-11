#ifndef INC_MM_H
#define INC_MM_H

#include "types.h"

void mm_init(void);
void *kalloc(void);
void kfree(void *va);
void inc_kmem_ref(uint64_t pa);
void dec_kmem_ref(uint64_t pa);
int get_kmem_ref(uint64_t pa);
uint64_t  get_totalram(void);
uint64_t  get_freeram(void);
void mm_test(void);
void mm_dump(void);

#endif
