# dashをdynamic linkでビルド

## ビルド

```
$ cd dash-0.5.11.5
$ make clean
$ CC=~/musl/bin/musl-gcc ./configure --host=aarch64-elf CFLAGS="-std=gnu99 -O3 -MMD -MP -fpie -Isrc" LDFLAGS="-pie"
```

## xv6に組み込み

- static link版dashをshにrename
- dynamic link版をdashとして使用
- `init`でinittabを実行するためのshellはstatic版でなければならない（この中で/lib/libc.soを/lib/ld-musl-aarch64.so.1にリンクしているので）

```
$ mv usr/bin/dash usr/bin/sh
$ cp dash-0.5.11.5/src/dash usr/bin/dash
$ vi usr/inc/files.h
$ rm -f fs.img
$ make
$ make qemu
```

### 実行

```
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
[0]fileopen: cant namei /etc/profile
[1]fileopen: cant namei /.profile
$ ls -l /
total 4
drwxrwxr-x 1 root root 1472 Jul 30  2022 bin
drwxrwxr-x 1 root root  384 Jul 30  2022 dev
drwxrwxr-x 1 root root  320 Jul 30  2022 etc
drwxrwxr-x 1 root root  192 Jul 30  2022 home
drwxrwxrwx 1 root root  256 Jul 30  2022 lib
-rwxr-xr-x 1 root root   94 Jul 30  2022 test.txt
drwxrwxr-x 1 root root  256 Jul 30  2022 usr
$ readelf -lhW /usr/bin/sh
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
  Entry point address:               0x4003b0
  Start of program headers:          64 (bytes into file)
  Start of section headers:          330584 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         3
  Size of section headers:           64 (bytes)
  Number of section headers:         23
  Section header string table index: 22

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  LOAD           0x001000 0x0000000000400000 0x0000000000400000 0x02b4b4 0x02b4b4 R E 0x1000
  LOAD           0x02c4c0 0x000000000042c4c0 0x000000000042c4c0 0x001790 0x002f08 RW  0x1000
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10

 Section to Segment mapping:
  Segment Sections...
   00     .init .text .fini .rodata
   01     .data.rel.ro .got .got.plt .data .bss
   02
$ readelf -lWh /usr/bin/dash
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
  Entry point address:               0x41fc
  Start of program headers:          64 (bytes into file)
  Start of section headers:          139808 (bytes into file)
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
  LOAD           0x000000 0x0000000000000000 0x0000000000000000 0x01be42 0x01be42 R E 0x10000
  LOAD           0x01be50 0x000000000002be50 0x000000000002be50 0x0015a0 0x002610 RW  0x10000
  DYNAMIC        0x01cb00 0x000000000002cb00 0x000000000002cb00 0x000170 0x000170 RW  0x8
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10

 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .interp .hash .dynsym .dynstr .rela.dyn .rela.plt .init .plt .text .fini .rodata
   03     .data.rel.ro .dynamic .got .got.plt .data .bss
   04     .dynamic
   05
$
```


## 更新履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/dash_dynamic.md
	modified:   usr/inc/files.h
	modified:   usr/src/init/main.c
	renamed:    usr/src/sh/main.c -> usr/src/mysh/main.c
```
