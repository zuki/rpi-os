//
// synchronize64.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright(C) 2014-2020  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef INC_SYNCHRONIZE_H
#define INC_SYNCHRONIZE_H

#include "types.h"

#define ARM_ALLOW_MULTI_CORE    1

#define CORES                   4

//
// Execution levels
//
#define TASK_LEVEL      0        // IRQs と FIQs が有効
#define IRQ_LEVEL       1        // IRQsは無効、FIQsは有効
#define FIQ_LEVEL       2        // IRQs と FIQs が無効

unsigned current_execution_level(void);

//
// 割り込み制御
//
#define    EnableIRQs()         asm volatile("msr DAIFClr, #2")
#define    DisableIRQs()        asm volatile("msr DAIFSet, #2")
#define    EnableFIQs()         asm volatile("msr DAIFClr, #1")
#define    DisableFIQs()        asm volatile("msr DAIFSet, #1")

// EnterCritical()はnTargetLevelが同じか大きいネストは可能
void enter_critical(unsigned level);    // Default: IRQ_LEVEL
void leave_critical(void);

//
// キャッシュ制御
//
#define DATA_CACHE_LINE_LENGTH_MIN      64    // CTR_EL0から
#define DATA_CACHE_LINE_LENGTH_MAX      64

#define invalidate_instruction_cache()    asm volatile("ic iallu" ::: "memory")
#define flush_prefetch_buffer()           asm volatile("isb" ::: "memory")

// 注: データキャッシュの操作にはDataSyncBarrierを含む
void invalidate_data_cache(void) MAXOPT;
void invalidate_data_cache_l1_only(void) MAXOPT;
void clean_data_cache(void) MAXOPT;

void invalidate_data_cache_range(uint64_t nAddress, uint64_t nLength) MAXOPT;
void clean_data_cache_range(uint64_t nAddress, uint64_t nLength) MAXOPT;
void clean_and_invalidate_data_cache_range(uint64_t nAddress, uint64_t nLength) MAXOPT;

void sync_data_and_instruction_cache(void);

//
// キャッシュアライメント
//
#define CACHE_ALIGN                     GALIGN(DATA_CACHE_LINE_LENGTH_MAX)

#define CACHE_ALIGN_SIZE(type, num)    ((( ((num)*sizeof(type) - 1)        \
                            |(DATA_CACHE_LINE_LENGTH_MAX-1)    \
                        ) + 1) / sizeof(type))

#define IS_CACHE_ALIGNED(ptr, size)    (((uintptr_t)(ptr) &(DATA_CACHE_LINE_LENGTH_MAX-1)) == 0 \
                        &&((size) &(DATA_CACHE_LINE_LENGTH_MAX-1)) == 0)

#define DMA_BUFFER(type, name, num)     type name[CACHE_ALIGN_SIZE(type, num)] CACHE_ALIGN

//
// バリア
//
#define DataSyncBarrier()           asm volatile("dsb sy" ::: "memory")
#define DataMemBarrier()            asm volatile("dmb sy" ::: "memory")

#define InstructionSyncBarrier()    asm volatile("isb" ::: "memory")
#define InstructionMemBarrier()     asm volatile("isb" ::: "memory")

#define CompilerBarrier()           asm volatile("" ::: "memory")

#define PeripheralEntry()          ((void) 0)    // ここでは無視
#define PeripheralExit()           ((void) 0)

//
// 割り込みとイベントを待つ
//
#define WaitForInterrupt()          asm volatile("wfi")
#define WaitForEvent()              asm volatile("wfe")
#define SendEvent()                 asm volatile("sev")

#endif
