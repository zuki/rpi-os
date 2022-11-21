#include "mmu.h"
#include "memlayout.h"
#include "sysregs.h"

.section ".text.boot"
.global _start
_start:
    /*
     * 0以外のプロセッサを起こす
     * start.elf (または bootcode.bin ?)に組み込まれている
     * https://github.com/raspberrypi/tools/blob/master/armstubs/armstub8.S を参照
     */
    adr     x0, mp_start
    mov     x1, #1
    mov     x2, #2
    mov     x3, #3
    mov     x9, 0xd8
    str     x0, [x9, x1, lsl #3]
    str     x0, [x9, x2, lsl #3]
    str     x0, [x9, x3, lsl #3]

    dsb     sy
    isb

    sev

mp_start:
    /* CurrentEL[3:2]から現在の例外レベルを取得する
     *     0x0000: EL0, 0x0100: EL1, 0x1000: EL2, 0x1100: EL3
     */
    mrs     x9, CurrentEL
    and     x9, x9, #0xc
    cmp     x9, #8
    beq     el2
    bgt     el3

el3:
    mov     x9, #SCR_VALUE
    msr     scr_el3, x9
    mov     x9, #SPSR_EL3_VALUE
    msr     spsr_el3, x9
    adr     x9, el2
    msr     elr_el3, x9
    eret

el2:
    /*
     * HCR_EL2.RW, bit[31] = 1: EL1の例外状態はAArch64.
     * EL0の例外状態はEL0で実行している際のPSTATE.nRWの
     * 現在値により決定される.
     */
    mov     x9, #HCR_VALUE
    msr     hcr_el2, x9

    /* SCTLR アクセスをセットアップ: Circle定義と一部異なる */
    /* MMU（アドレス変換）を無効に */
    ldr     x9, =SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x9

    /* SIMD命令を有効にする: Circleではcptr_el2とhstr_el2も設定している */
    ldr     x9, =CPACR_VALUE
    msr     cpacr_el1, x9

    /* 例外レベルをEL1に変更する: CircleはEL1t, これはEL1h) */
    mov     x9, #SPSR_EL2_VALUE
    msr     spsr_el2, x9
    adr     x9, el1
    msr     elr_el2, x9
    eret

el1:
    // kpgdir = 0x00 (物理メモリ)
    adr     x9, kpgdir

    /* 高位ハーフと低位ハーフを同じ物理メモリ領域ににマップする. */
    msr     ttbr0_el1, x9
    msr     ttbr1_el1, x9

    ldr     x9, =TCR_VALUE
    msr     tcr_el1, x9

    ldr     x9, =MAIR_VALUE
    msr     mair_el1, x9

    isb

    /* MMUを有効に */
    mrs     x9, sctlr_el1
    orr     x9, x9, #SCTLR_MMU_ENABLED
    msr     sctlr_el1, x9

    isb

    /* CPU IDを読み込む */
    mrs     x0, mpidr_el1
    and     x0, x0, #3

    /* 各CPU用のスタックを用意する */
    /* sp = _start - cpuid() * PGSIZE */
    mov     x10, #KSTACKSIZE
    mul     x10, x10, x0
    ldr     x9, =_start
    sub     x9, x9, x10

    /* スタックポインタを設定する。常に16バイトアラインでなければならない */
    /* 例外レベルELx専用のSP_ELxを使用する: ELxh */
    msr     spsel, #1
    mov     sp, x9
    /* mainに分岐 */
    ldr     x9, =main
    br      x9
