# binutilsをdynamic linkで導入

## ビルド

```
$ wget http://ftp.gnu.org/gnu/binutils/binutils-2.38.tar.xz
# tar Jxf binutils-2.38.tar.xz
$ cd binutils-2.38
$ CC=$HOME/musl/bin/musl-gcc ./configure -host=aarch64-elf --disable-nls --prefix=$HOME/bintuils CFLAGS="-O3 -fpie"
$ make configure-host
$ make LDFLAGS="-pie"
$ make install

$ file $HOME/binutils/bin/readelf
$HOME/binutils/bin/readelf: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped

$ aarch64-elf-readelf -hlW $HOME/binutils/bin/readelf
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Position-Independent Executable file)
  Machine:                           AArch64
  Version:                           0x1
  Entry point address:               0xb61c
  Start of program headers:          64 (bytes into file)
  Start of section headers:          1069720 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         6
  Size of section headers:           64 (bytes)
  Number of section headers:         22
  Section header string table index: 21

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  PHDR           0x000040 0x0000000000000040 0x0000000000000040 0x000150 0x000150 R   0x8
  INTERP         0x000190 0x0000000000000190 0x0000000000000190 0x00001a 0x00001a R   0x1
      [Requesting program interpreter: /lib/ld-musl-aarch64.so.1]
  LOAD           0x000000 0x0000000000000000 0x0000000000000000 0x0f1c50 0x0f1c50 R E 0x10000
  LOAD           0x0f1c50 0x0000000000101c50 0x0000000000101c50 0x005aa8 0x009a38 RW  0x10000
  DYNAMIC        0x0f4fc0 0x0000000000104fc0 0x0000000000104fc0 0x000170 0x000170 RW  0x8
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10

 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .interp .hash .dynsym .dynstr .rela.dyn .rela.plt .init .plt .text .fini .rodata
   03     .data.rel.ro .dynamic .got .got.plt .data .bss
   04     .dynamic
   05
```

## xv6に導入

```
$ cp -r $HOME/binutils/bin/* usr/bin/
$ vi usr/inc/files.h
$ git diff usr/inc/files.h
diff --git a/usr/inc/files.h b/usr/inc/files.h
index 7778d23..4c8e023 100644
--- a/usr/inc/files.h
+++ b/usr/inc/files.h
@@ -111,6 +111,22 @@ char *usrbins[] = {
     "usr/bin/paste",
     "usr/bin/nohup",
     "usr/bin/dash",
+    "usr/bin/addr2line",
+    "usr/bin/as",
+    "usr/bin/elfedit",
+    "usr/bin/ld",
+    "usr/bin/nm",
+    "usr/bin/objdump",
+    "usr/bin/readelf",
+    "usr/bin/strings",
+    "usr/bin/ar",
+    "usr/bin/c++filt",
+    "usr/bin/gprof",
+    "usr/bin/ld.bfd",
+    "usr/bin/objcopy",
+    "usr/bin/ranlib",
+    "usr/bin/size",
+    "usr/bin/strip",
     NULL
 };


$ rm -f obj/fs.img
$ make
$ make qemu

Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
[3]fileopen: cant namei /etc/profile
[1]fileopen: cant namei /.profile
$ readelf -hl /bin/ls
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           AArch64
  Version:                           0x1
  Entry point address:               0x400088
  Start of program headers:          64 (bytes into file)
  Start of section headers:          52232 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         3
  Size of section headers:           64 (bytes)
  Number of section headers:         13
  Section header string table index: 12

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000001000 0x0000000000400000 0x0000000000400000
                 0x00000000000088e8 0x00000000000088e8  R E    0x1000
  LOAD           0x00000000000098e8 0x00000000004098e8 0x00000000004098e8
                 0x0000000000000230 0x00000000000009f0  RW     0x1000
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10

 Section to Segment mapping:
  Segment Sections...
   00     .init .text .fini .rodata
   01     .got .got.plt .data .bss
   02
$
```

## objdumpでエラー

```
$ objdump -d /bin/ls

/bin/ls:     file format elf64-littleaarch64

CurrentEL: 0x1
DAIF: Debug(1) SError(1) IRQ(1) FIQ(1)
SPSel: 0x1
SPSR_EL1: 0x0
SP: 0xffff00003addbe00
SP_EL0: 0xfffffffffa80
ELR_EL1: 0xc0000003e0c8, EC: 0x0, ISS: 0x0.
FAR_EL1: 0x0
Unexpected syscall #233 (sys_madvise)           // syscall未実装
kern/console.c:347: kernel panic at cpu 3.
QEMU: Terminated
```

### `sys_advise()`を実装で解決

```
$ objdump -d /bin/ls

/bin/ls:     file format elf64-littleaarch64


Disassembly of section .init:

0000000000400000 <_init>:
  400000:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
  400004:	910003fd 	mov	x29, sp
  400008:	a8c17bfd 	ldp	x29, x30, [sp], #16
  40000c:	d65f03c0 	ret

Disassembly of section .text:

0000000000400010 <exit>:
  400010:	a9bf7bf3 	stp	x19, x30, [sp, #-16]!
  400014:	2a0003f3 	mov	w19, w0
  400018:	94000232 	bl	4008e0 <__funcs_on_exit>
  40001c:	94000232 	bl	4008e4 <__libc_exit_fini>
  400020:	940014be 	bl	405318 <__stdio_exit>
  400024:	2a1303e0 	mov	w0, w19
  400028:	94000d3d 	bl	40351c <_Exit>
  40002c:	d503201f 	nop

0000000000400030 <main>:
  400030:	a9be7bfd 	stp	x29, x30, [sp, #-32]!
  400034:	910003fd 	mov	x29, sp
  400038:	7100041f 	cmp	w0, #0x1
  40003c:	540001ed 	b.le	400078 <main+0x48>
...
Disassembly of section .fini:

0000000000407c10 <_fini>:
  407c10:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
  407c14:	910003fd 	mov	x29, sp
  407c18:	a8c17bfd 	ldp	x29, x30, [sp], #16
  407c1c:	d65f03c0 	ret
$

$ objdump -d /bin/hello-dyn

/bin/hello-dyn:     file format elf64-littleaarch64


Disassembly of section .init:

0000000000000338 <_init>:
 338:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
 33c:	910003fd 	mov	x29, sp
 340:	a8c17bfd 	ldp	x29, x30, [sp], #16
 344:	d65f03c0 	ret

Disassembly of section .plt:

0000000000000348 <.plt>:
 348:	a9bf7bf0 	stp	x16, x30, [sp, #-16]!
 34c:	b0000010 	adrp	x16, 1000 <_fini+0xc0c>
 350:	f942e211 	ldr	x17, [x16, #1472]
 354:	91170210 	add	x16, x16, #0x5c0
 358:	d61f0220 	br	x17
...
Disassembly of section .fini:

00000000000003f4 <_fini>:
 3f4:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
 3f8:	910003fd 	mov	x29, sp
 3fc:	a8c17bfd 	ldp	x29, x30, [sp], #16
 400:	d65f03c0 	ret
$
```

## 変更履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	modified:   .gitignore
	modified:   inc/syscall1.h
	modified:   kern/syscall.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/bintuils.md
	modified:   usr/inc/files.h
```
