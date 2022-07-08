#ifndef INC_VM_H
#define INC_VM_H

#include "types.h"
#include "proc.h"

uint64_t *  vm_init();
void        vm_free(uint64_t *pgdir);
uint64_t *  pgdir_walk(uint64_t * pgdir, void *vap, int alloc);
uint64_t *  uvm_copy(uint64_t *pgdir);
uint64_t *  uvm_copy2(struct proc *p);
void        uvm_unmap(uint64_t *pgdir, uint64_t va, uint64_t npages, int do_free);
void        uvm_switch(uint64_t *pgdir);
int         uvm_map(uint64_t *pgdir, void *va, size_t sz, uint64_t pa, uint64_t perm);
int         uvm_alloc(uint64_t *pgdir, size_t base, size_t stksz, size_t oldsz, size_t newsz);
int         uvm_dealloc(uint64_t *pgdir, size_t base, size_t oldsz, size_t newsz);

int         copyout(uint64_t *pgdir, void *va, void *p, size_t len);

void        vm_stat(uint64_t *);
void        vm_test();

#endif
