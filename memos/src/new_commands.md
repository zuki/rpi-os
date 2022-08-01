# 各種コマンドの追加

## 1. `file`コマンド

### ビルドとxv6への導入

```
$ wget https://astron.com/pub/file/file-5.40.tar.gz
$ tar xf file-5.40.tar.gz
$ cd file-5.40
$ CC=~/musl/bin/musl-gcc ./configure --host=aarch64-elf CFLAGS="-std=gnu99 -O3 -MMD -MP -fpie -Isrc" LDFLAGS="-pie"
$ make

$ cd XV6
$ mkdir -p usr/local/bin
$ mkdir -p usr/local/share/misc
$ cp $FILE/src/file usr/local/bin
$ cp $FILE/magic/magic.mgc /usr/local/share/misc
$ vi usr/inc/files.h
$ vi usr/src/mkfs/main.c
$ rm -rf obj
$ make
```

### 実行

```
$ file /bin/ls
CurrentEL: 0x1
DAIF: Debug(1) SError(1) IRQ(1) FIQ(1)
SPSel: 0x1
SPSR_EL1: 0x40000000
SP: 0xffff0000389b1e00
SP_EL0: 0xffffffffec10
ELR_EL1: 0xc00000105c90, EC: 0x0, ISS: 0x0.
FAR_EL1: 0x0
Unexpected syscall #67 (unknown)                // sys_pread64が未実装
```

### sys_pread64()を実装

- 正常に動くが、magicファイルがキャッシュされず毎回7MB近いファイルを読むので非常に時間がかかる
- filereadにpage_cacheを使うようにしたらloginが動かなかった。とりあえずTODO。

```
$ file /bin/ls
/bin/ls: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, not stripped
$ file /usr/bin/dash
/usr/bin/dash: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped
$ file /bin/mysh
/bin/mysh: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, not stripped
$
```

### 1. 変更履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	modified:   inc/file.h
	modified:   inc/syscall1.h
	modified:   kern/file.c
	modified:   kern/syscall.c
	modified:   kern/sysfile.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/new_commands.md
	modified:   usr/inc/files.h
	new file:   usr/local/share/misc/magic.mgc
	modified:   usr/src/mkfs/main.c
```

## 2. gawk

```
$ wget https://ftp.jaist.ac.jp/pub/GNU/gawk/gawk-5.1.0.tar.xz
$ tar Jxf gawk-5.1.0.tar.xz
$ cd gawk-5.1.0
$ CC=~/musl/bin/musl-gcc ./configure --host=aarch64-elf --disable-nls CFLAGS="-std=gnu99 -O3 -MMD -MP -fpie -Isrc" LDFLAGS="-pie"
$ make
$ $ file gawk
gawk: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped

$ cp gawk $XV6/usr/bin
$ vi usr/inc/files.h
$ rm -rf obj
$ make
$ maek qemu
```

### 実行

```
$ echo "this line of data is ignored" > test
$ gawk '{print "Hello, world" }' test
Hello, world
$ echo "Hello, world" > test2
$ gawk '{print}' test2
Hello, world
$ gawk 'BEGIN { print "Hello, world" }'
Hello, world
$ echo '/^$/ { print "This is a blank line." }' > awksrc
$ gawk -f awksrc test
$ echo "\n\n\n" > test
$ gawk -f awksrc test
This is a blank line.
This is a blank line.
This is a blank line.
This is a blank line.
$ gawk -f bitmap.awk bitmap.test
XOOOOOOOOOOX
OXOOOOOOOOXO
OOXOOOOOOXOO
OOOXOOOOXOOO
OOOOXOOXOOOO
OOOOOXXOOOOO
OOOOOXXOOOOO
OOOOXOOXOOOO
OOOXOOOOXOOO
OOXOOOOOOXOO
OXOOOOOOOOXO
XOOOOOOOOOOX
$ gawk -f grades.awk grades.test
mona	79	C
john	88	B
andrea	90.5	A
jasper	85	B
dunce	64.5	D
ellis	93.5	A

Class Average: 	83.4167
At or Above Average: 	4
Below Average: 	2
[2]sys_clone: flags other than SIGCHLD are not supported
gawk: grades.awk:48: (FILENAME=grades.test FNR=6) fatal: cannot open pipe `sort' for output: Operation not permitted
$ su root
# ln -s /usr/bin/gawk /usr/bin/awk
# ls -l /usr/bin/awk
lrwxrwxrwx 1 root root 13 Jun 21 10:32 /usr/bin/awk -> /usr/bin/gawk
# exit
$ ./awkro
[3]execve: elf header magic invalid
[3]execve: bad
-dash: 11: ./awkro: Permission denied
$ dash awkro
awkro: 1: awk: Permission denied
$ awk { print "hello, wolrd" } acronyms
-dash: 15: awk: Permission denied
```

**注**: 「sed & awkプログラミング 改訂版」のソースを使用

### gawkにシンボリックリンクした`awk`が動かない

- `statat`と`execve`でシンボリックリンクをたどっていなかったため
- 両関数を修正

```
$ awk '{print "hello"}' acronyms
[1]sys_fstatat: dirfd: -100, path: /usr/local/sbin/awk, st: 0xfffffffffb10, flags: 0
[1]sys_fstatat: dirfd: -100, path: /usr/local/bin/awk, st: 0xfffffffffb10, flags: 0
[0]sys_fstatat: dirfd: -100, path: /usr/sbin/awk, st: 0xfffffffffb10, flags: 0
[1]sys_fstatat: dirfd: -100, path: /usr/bin/awk, st: 0xfffffffffb10, flags: 0	// ここでgawkを見つける
[0]execve: [12] parse /usr/bin/awk		// execveはawkで呼び出し
hello
hello
hello
```

- デバッグプリントを削除

```
$ ls
acronyms  bitmap.awk   date-month  grades.awk	lookup
awkro	  bitmap.test  factorial   grades.test	sample
$ dash awkro sample
The U.S. Global Change Research Program (USGCRP) is a comprehensive
research effort that includes applied
as well as basic research.
The National Aeronautic and Space Administration (NASA) program Mission to Planet Earth
represents the principal space-based component
of the U.S. Global Change Research Program (USGCRP) and includes new initiatives
such as Earth Observing System (EOS) and Earthprobes.
$ echo 08/01/99 | dash date-month
August 01, 1999
[3]fileopen: cant namei /dev/null
$ echo 7 | dash factorial
Enter number: The factorial of 7 is 5040
[2]fileopen: cant namei /dev/null
$ awk -f bitmap.awk bitmap.test
XOOOOOOOOOOX
OXOOOOOOOOXO
OOXOOOOOOXOO
OOOXOOOOXOOO
OOOOXOOXOOOO
OOOOOXXOOOOO
OOOOOXXOOOOO
OOOOXOOXOOOO
OOOXOOOOXOOO
OOXOOOOOOXOO
OXOOOOOOOOXO
XOOOOOOOOOOX
$ awk -f grades.awk grades.test
mona	79.5	C
john	88	B
andrea	90.5	A
jasper	85	B
dunce	64.5	D
ellis	93.5	A

Class Average: 	83.5
At or Above Average: 	4
Below Average: 	2
[2]sys_clone: flags 0x4111, child stack 0xfffffffff8a0
[0]sys_clone: flags other than SIGCHLD are not supported
awk: grades.awk:48: (FILENAME=grades.test FNR=6) fatal: cannot open pipe `sort' for output: Operation not permitted
$
```

- sys_cloneエラーはTODO

### 2. 更新履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	new file:   home/zuki/ch08/acronyms
	new file:   home/zuki/ch08/awkro
	new file:   home/zuki/ch08/bitmap.awk
	new file:   home/zuki/ch08/bitmap.test
	new file:   home/zuki/ch08/date-month
	new file:   home/zuki/ch08/factorial
	new file:   home/zuki/ch08/grades.awk
	new file:   home/zuki/ch08/grades.test
	new file:   home/zuki/ch08/lookup
	new file:   home/zuki/ch08/sample
	modified:   inc/linux/fcntl.h
	modified:   kern/exec.c
	modified:   kern/syscall.c
	modified:   kern/sysfile.c
	modified:   kern/sysproc.c
	modified:   memos/src/new_commands.md
	modified:   usr/etc/inittab
	modified:   usr/inc/files.h
	modified:   usr/src/mkfs/main.c
```
