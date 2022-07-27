#ifndef INC_EXEC_H
#define INC_EXEC_H

#include "types.h"
#include "memlayout.h"
#include "linux/elf.h"
#include "linux/elf-em.h"
#include "linux/capability.h"

#define ELF_MIN_ALIGN       4096
#define ELF_PAGESTART(_v)   ((_v) & ~(uint64_t)(ELF_MIN_ALIGN-1))
#define ELF_PAGEOFFSET(_v)  ((_v) & (uint64_t)(ELF_MIN_ALIGN-1))
#define ELF_PAGEALIGN(_v)   (((_v) + ELF_MIN_ALIGN - 1) & ~(uint64_t)(ELF_MIN_ALIGN - 1))
#define ELF_ET_DYN_BASE     (3 * USERTOP / 4)      // 0xc000_00000000
#define ELF_EXEC_PAGESIZE   4096
#define BINPRM_BUF_SIZE     128

#define BAD_ADDR(x)         ((uint64_t)(x) > USERTOP)
// AUXVの個数
#define ELF_CHECK_ARCH(x)   ((x)->e_machine == EM_AARCH64)

// 以下はaarch64版ubuntuの値を使用: arch/arm64/include/uapi/asm/hwcap.h
// 0x887 = FP, ASIMD, EVTSTRM, PMULL, SHA1, SHA2, ATOMICS, FPHP
#define ELF_HWCAP           0x887
#define ELF_HWCAP2          0
#define ELF_PLATFORM        "aarch64"

#ifndef elf_addr_t
#define elf_addr_t          uint64_t
#define elf_caddr_t         char *
#endif

#endif
