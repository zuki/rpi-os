#ifndef INC_MMAP_H
#define INC_MMAP_H

#include "file.h"
#include "proc.h"
#include "types.h"

#define NOT_PAGEALIGN(a)  ((uint64_t)(a) & (PGSIZE-1))

long mmap(void *, size_t, int, int, struct file*, off_t);
long munmap(void *, size_t);
void *mremap(void *, size_t, size_t, int, void *);
long msync(void *, size_t, int);

uint64_t get_perm(int, int);
void free_mmap_list(struct proc *);
long mmap_load_pages(void *, size_t, int, int, struct file*, off_t);
void print_mmap_list(struct proc *, const char *);
long copy_mmap_list(struct proc *, struct proc *);
long copy_mmap_list2(struct proc *, struct proc *);
long copy_mmap_pages(void *, size_t, uint64_t);
void print_vmas(struct proc *);
struct mmap_region *find_available_region(void *);
struct mmap_region *find_mmap_region(void *);
//long scale_mmap_region(struct mmap_region *, uint64_t);
void ref_inc(struct mmap_region *);
void ref_dec(struct mmap_region *);
int  ref_get(struct mmap_region *);

#endif
