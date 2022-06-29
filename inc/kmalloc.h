#ifndef INC_KMALLOC_H
#define INC_KMALLOC_H

#include "types.h"

void kmfree(void *ap);
void *kmalloc(size_t nbytes);

#endif
