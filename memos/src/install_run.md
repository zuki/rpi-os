# 01: インストールと実行

- ubuntu 20.04のqemu-system-aarch64(v4.2)にはraspi3bがない
- ubuntu 22.04にアップグレード(v6.2)
- 問題なく動いた

```
$ make

$ make qemu
make -C boot
make[1]: Entering directory '/home/vagrant/rpi-os/boot'
make[1]: Nothing to be done for 'all'.
make[1]: Leaving directory '/home/vagrant/rpi-os/boot'
make -C usr
make[1]: Entering directory '/home/vagrant/rpi-os/usr'
make -C ../libc
make[2]: Entering directory '/home/vagrant/rpi-os/libc'
make[2]: Nothing to be done for 'all'.
make[2]: Leaving directory '/home/vagrant/rpi-os/libc'
mkdir -p ../obj/usr/
# Replace "/usr/local/musl" to "../libc"
sed -e "s/\/usr\/local\/musl/..\/libc/g" ../libc/lib/musl-gcc.specs > ../obj/usr/musl-gcc.specs
make ../obj/usr/bin/mkfs ../obj/usr/bin/init ../obj/usr/bin/cat ../obj/usr/bin/ls ../obj/usr/bin/sh ../obj/usr/bin/utest ../obj/usr/bin/echo
make[2]: Entering directory '/home/vagrant/rpi-os/usr'
make[2]: '../obj/usr/bin/mkfs' is up to date.
make[2]: '../obj/usr/bin/init' is up to date.
make[2]: '../obj/usr/bin/cat' is up to date.
make[2]: '../obj/usr/bin/ls' is up to date.
make[2]: '../obj/usr/bin/sh' is up to date.
make[2]: '../obj/usr/bin/utest' is up to date.
make[2]: '../obj/usr/bin/echo' is up to date.
make[2]: Leaving directory '/home/vagrant/rpi-os/usr'
make[1]: Leaving directory '/home/vagrant/rpi-os/usr'
make obj/sd.img
make[1]: Entering directory '/home/vagrant/rpi-os'
make[1]: 'obj/sd.img' is up to date.
make[1]: Leaving directory '/home/vagrant/rpi-os'
qemu-system-aarch64 -M raspi3b -nographic -serial null -serial mon:stdio -drive file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
[3]free_range: 0xffff0000000a6000 ~ 0xffff00003c000000, 245594 pages
[3]main: cpu 3 init finished
[1]main: cpu 1 init finished
[2]main: cpu 2 init finished
[0]main: cpu 0 init finished
[3]emmc_card_init: poweron
[3]emmc_card_reset: control0: 0x0, control1: 0x0, control2: 0x0
[3]emmc_card_reset: status: 0x1ff0000
[3]emmc_get_base_clock: base clock rate is 50000000 Hz
[3]emmc_get_clock_divider: base_clock: 50000000, target_rate: 400000, divisor: 0x40, actual_clock: 390625, ret: 0x4000
[3]emmc_issue_command_int: rrror occured whilst waiting for command complete interrupt
[3]emmc_card_reset: OCR: 0xffff, 1.8v support: 0, SDHC support: 0
[3]emmc_get_clock_divider: base_clock: 50000000, target_rate: 25000000, divisor: 0x1, actual_clock: 25000000, ret: 0x100
[3]emmc_card_reset: card CID: 0xaa5859, 0x51454d55, 0x2101dead, 0xbeef0062
[3]emmc_card_reset: RCA: 0x4567
[3]emmc_card_reset: SCR: version 2.00, bus_widths 0x5
[3]emmc_card_reset: found valid version 2.00 SD card
[3]dev_init: LBA of 1st block 0x20800, 0x1f800 blocks totally
[3]iinit: sb: size 1000 nblocks 941 ninodes 200 nlog 30 logstart 2 inodestart 32 bmapstart 58
init: starting sh
sh: argv[0] = 'sh'
sh: testenv = 'FROM_INIT'
$ ls
.              4000 1 512
..             4000 1 512
mkfs           8000 2 45448
init           8000 3 23264
cat            8000 4 39392
ls             8000 5 43936
sh             8000 6 57096
utest          8000 7 16800
echo           8000 8 39416
console        0 9 0
$ echo abc
abc
$ utest
$ QEMU 6.2.0 monitor - type 'help' for more information
(qemu) q
```

## `-DDEBUG`を定義するとメモリ不足でpanic

- `DLOG_ERROR`にさげてもだめ
- -DDEBUGで有効になる`mm.c`の以下のassertが失敗

    ```
    93:        for (int i = 8; i < PGSIZE; i++) {
    94:            assert(*(char *)(p + i) == 0xAC);
    95        }
    ```

```
file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
[0]free_range: 0xffff0000009c8000 ~ 0xffff00003c000000, 243256 pages
[0]mbox_test: pass
mm: no more space for debug. kern/console.c:286: kernel panic at cpu 0.
```

# 実機で実行

問題なく実行できた。

```
Welcome to minicom 2.8

OPTIONS:
Compiled on Jan  4 2021, 00:04:46.
Port /dev/cu.usbserial-AI057C9L, 10:46:52
Using character set conversion

Press Meta-Z for help on special keys

[0]free_range: 0xffff0000000a6000 ~ 0xffff00003b400000, 242522 pages
[0]main: cpu 0 init finished
[1]main: cpu 1 init finished
[2]main: cpu 2 init finished
[0]emmc_card_init: poweron
[3]main: cpu 3 init finished
[0]emmc_card_reset: control0: 0x0, control1: 0x0, control2: 0x0
[0]emmc_card_reset: status: 0x1fff0000
[0]emmc_get_base_clock: base clock rate is 200000000 Hz
[0]emmc_get_clock_divider: base_clock: 200000000, target_rate: 400000, divisor:0
[0]emmc_issue_command_int: rrror occured whilst waiting for command complete int
[0]emmc_card_reset: OCR: 0xff80, 1.8v support: 0, SDHC support: 1
[0]emmc_get_clock_divider: base_clock: 200000000, target_rate: 25000000, diviso0
[0]emmc_card_reset: card CID: 0x9f5449, 0x20202020, 0x20108400, 0x5bf015a
[0]emmc_card_reset: RCA: 0x59b4
[0]emmc_card_reset: SCR: version 4.xx, bus_widths 0x5
[0]emmc_card_reset: found valid version 4.xx SD card
[0]dev_init: LBA of 1st block 0x20800, 0x1f800 blocks totally
[0]iinit: sb: size 1000 nblocks 941 ninodes 200 nlog 30 logstart 2 inodestart 38
init: starting sh
sh: argv[0] = 'sh'
sh: testenv = 'FROM_INIT'
$ ls
.              4000 1 512
..             4000 1 512
mkfs           8000 2 45448
init           8000 3 23264
cat            8000 4 39392
ls             8000 5 43936
sh             8000 6 57096
utest          8000 7 16800
echo           8000 8 39416
console        0 9 0
test           8000 10 4
$ echo abc
abc
$ echo abc > test.txt
$ cat test.txt
abc
$ `Cntl-P`を押下
1 sleep  init
2 run    idle
3 runble idle
4 run    idle
5 run    idle
6 sleep  sh fa: 1
$ exit
[0]execve: namei bad
[0]execve: bad
exec ?exit failed
$
```

## [https://musl.cc/]{https://musl.cc/}版のgcc+musl環境を使用

ユーザプログラムをデバッグモーでコンパイルできるようなので試してみた

### aarch64-linux-musl-crossをインストール

```
$ wget https://musl.cc/aarch64-linux-musl-cross.tgz
$ tar xf aarch64-linux-musl-cross.tgz
$ vi .bashrc
export PATH="/home/vagrant/aarch64-linux-musl-cross/bin":$PATH
$ source .bashrc
```

### coreutilsとdashを再コンパイル

```
$ cd $COREUTILS
$ make clean
$ CC=aarch64-linux-musl-gcc ./configure --host=aarch64-linux-musl CFLAGS="-std=gnu99 -g -O3 -MMD -MP -static -fno-plt -fno-pic -fpie -z max-page-size=4096"
$ make
$ file src/ls
src/ls: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), static-pie linked, with debug_info, not stripped
$ aarch64-linux-musl-objdump -d -S coreutils-8.32/src/ls
0000000000003160 <main>:
...
int
main (int argc, char **argv)
{
    3160:	a9b37bfd 	stp	x29, x30, [sp, #-208]!
    3164:	910003fd 	mov	x29, sp
    3168:	a9046bf9 	stp	x25, x26, [sp, #64]
  int i;
  struct pending *thispend;
  int n_files;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
    316c:	9000023a 	adrp	x26, 47000 <long_options+0x2c8>
{
    3170:	a90153f3 	stp	x19, x20, [sp, #16]
    3174:	aa0103f3 	mov	x19, x1
    3178:	2a0003f4 	mov	w20, w0
  set_program_name (argv[0]);
    317c:	f9400020 	ldr	x0, [x1]
$ cd src && find . -type f -executable -not -name '*.so' -exec cp {} ../../usr/bin/ \;

$ cd $DASH
$ make clean
$ CC=aarch64-linux-musl-gcc ./configure --host=aarch64-linux-musl --enable-static CFLAGS="-std=gnu99 -g -O3 -MMD -MP -static -fno-plt -fno-pic -fpie -z max-page-size=4096 -Isrc"
$ make
$ file src/dash
src/dash: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), static-pie linked, with debug_info, not stripped
$ aarch64-linux-musl-objdump -S -d src/dash
0000000000003970 <main>:
...
int
main(int argc, char **argv)
{
    3970:	a9b77bfd 	stp	x29, x30, [sp, #-144]!
    3974:	910003fd 	mov	x29, sp
    3978:	a90153f3 	stp	x19, x20, [sp, #16]

#if PROFILE
	monitor(4, etext, profile_buf, sizeof profile_buf, 50);
#endif
	state = 0;
	if (unlikely(setjmp(main_handler.loc))) {
    397c:	f0000194 	adrp	x20, 36000 <signal_names+0x1d0>
{
    3980:	f90027e1 	str	x1, [sp, #72]
$ cp src/dash ../usr/bin/
```



3. developブランチににmacブランチをマージ

```
$ cd $XV6
$ git checkout develop
$ git fetch origin mac
$ git merge origin/mac
$ コンフリクトを修正
$ git add .
$ git commit -m "merge mac branch"
$ vi Makefile
-MUSL_INC = /Users/dspace/musl/include
+MUSL := /home/vagrant/aarch64-linux-musl-cross/aarch64-linux-musl
+MUSL_INC = $(MUSL)/include
$ vi config.mk
-CROSS := aarch64-elf-
+CROSS := aarch64-linux-musl-
$ vi mksd.mk
- mformat -F -c 1 -i $@ ::
+ mkfs.vfat -F 32 -s 1 $@
$ vi usr/Makefile
-MUSL = /Users/dspace/musl
-LIBC_A = $(MUSL)/libc/lib/libc.a
-LIBC_SPEC = $(MUSL)/lib/musl-gcc.specs
+MUSL = /home/vagrant/aarch64-linux-musl-cross/aarch64-linux-musl
+LIBC_A = $(MUSL)/lib/libc.a
+# LIBC_SPEC = $(MUSL)/lib/musl-gcc.specs

-USR_CC := $(MUSL)/bin/musl-gcc
+USR_CC := $(CC)

-CFLAGS = -std=gnu99 -O3 -MMD -MP -static -z max-page-size=4096 \
-  -fno-omit-frame-pointer -Iinc/
+CFLAGS = -std=gnu99 -g -O3 -MMD -MP -static -z max-page-size=4096 \
+  -fno-omit-frame-pointer -Iinc

+LIBS_DEPS:
+       touch LIBC_BUILT
+
-$(OBJ)/%.c.o: %.c $(LIBC_DEPS)
+$(OBJ)/%.c.o: %.c

$ make
$ make qemu                     // すごい遅いが動く
# mmaptest2

[F-13] file backed shared mapping with fork test
buf2[0]=0xffff00003bb2c000
[F-13] failed at strcmp 3: buf2[0, 49, 50]=[, o, a], buf=[o, o, a]      // 結果は同じ

file_test:  ok: 0, ng: 1
```
