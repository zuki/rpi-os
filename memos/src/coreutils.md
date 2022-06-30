# 12: coreutilsを導入

## 入手と展開

```
$ wget https://ftp.jaist.ac.jp/pub/GNU/coreutils/coreutils-8.32.tar.xz
$ tar Jxf coreutils-8.32.tar.xz
```

## src以外はgitから無視する

```
$ vi .gitginore
coreutils-8.32/*
!coreutils-8.32/src
coreutils-8.32/src/*
!coreutils-8.32/src/*.[ch]
coreutils-8.32/src/coreutils.h
coreutils-8.32/src/version.[ch]
```

## srcを一部変更

```
$ git diff
diff --git a/coreutils-8.32/src/ls.c b/coreutils-8.32/src/ls.c
index 24b9832..dbd48c2 100644
--- a/coreutils-8.32/src/ls.c
+++ b/coreutils-8.32/src/ls.c
@@ -3023,7 +3023,7 @@ print_dir (char const *name, char const *realname, bool command_line_arg)
         {
           /* If readdir finds no directory entries at all, not even "." or
              "..", then double check that the directory exists.  */
-          if (syscall (SYS_getdents, dirfd (dirp), NULL, 0) == -1
+          if (syscall (SYS_getdents64, dirfd (dirp), NULL, 0) == -1
               && errno != EINVAL)
             {
               /* We exclude EINVAL as that pertains to buffer handling,
```

## configureとmake

```
$ cd coreutils-8.32
$ CC=$HOME/musl/bin/musl-gcc ./configure --host=aarch64-elf CFLAGS="-std=gnu99 -O3 -MMD -MP -static -fno-plt -fno-pic -fpie -z max-page-size=4096"
$ make
$ file src/ls
src/ls: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, with debug_info, not stripped
```

## xv6に組み込む

- `/usr/bin`に置く
- Makefileを使わず、`mkfs`コマンドで直接コピーする


### `usr/inc/usrbins.h`を作成

- findコマンドのオプションがLinuxとは異なる（BSD由来だから）ことに注意

```
$ find . -type f -perm +0111 -not -name 'make-prime-list' -not -name '*.so' > ../../usr/bin/core.txt
$ vi usr/inc/usrbins.h
```

### コマンドをコピー

```
$ find . -type f -perm +0111 -not -name 'make-prime-list' -not -name '*.so' -exec cp {} ../../usr/bin \;
```

# コマンド実行チェック

## ls

```
# ls -l
total 4
drwxrwxr-x 1 root root 896 Jun 30  2022 bin
drwxrwxr-x 1 root root 384 Jun 30  2022 dev
drwxrwxr-x 1 root root 256 Jun 30  2022 etc
drwxrwxr-x 1 root root 192 Jun 30  2022 home
drwxrwxrwx 1 root root 128 Jun 30  2022 lib
?--------- 1 root root   0 Jun 21 10:31 test
drwxrwxr-x 1 root root 256 Jun 30  2022 usr
```

## rm

```
// rm
# rm test
# ls
bin  dev  etc  home  lib  usr
```

## head

```
# head -n 5 README.md
# Raspberry Pi Operating System

Yet another unix-like toy operating system running on Raspberry Pi 3/4, which is built when I was preparing [labs](https://github.com/FDUCSLG/OS-2020Fall-Fudan/) for operating system course at Fudan University, following the classic framework of [xv6](https://github.com/mit-pdos/xv6-public/).

Tested on Raspberry Pi 3A+, 3B+, 4B.
```

## tail

```
# tail -n 5 README.md
├── inc: Kernel headers.
├── kern: Kernel source code.
└── usr: User programs.

```

### cat

```
# cat > test.txt
123
111
999
123
あいうえお
12345
111
999
あいうえお
かきくけこ        // Ctrl-D

# cat test.txt
123
111
999
123
あいうえお
12345
111
999
あいうえお
かきくけこ
```

### sort

```
# sort test.txt
111
111
123
123
12345
999
999
あいうえお
あいうえお
かきくけこ
```

## uniq

```
# uniq test.txt
123
111
123
999
あいうえお
999
かきくけこ
12345
あいうえお
# uniq -c test.txt
      1 123
      2 111
      1 123
      1 999
      2 あいうえお
      1 999
      1 かきくけこ
      1 12345
      1 あいうえお
#
```

## cp

```
# cp test.txt test2.txt
# ls -il test.txt test2.txt
 28 -rwxr-xr-x 1 root root 94 Jun 30  2022 test.txt
140 -rwxr-xr-x 1 root root 94 Jun 21  2022 test2.txt
```

## cut

```
# cut -c1-3 test.txt
123
111
111
123
999
あ
あ
999
か
123
あ
# cat > del.txt
123,123,234
234,345,123
987,123,789
# cut -d, -f2 del.txt
123
345
123
```

## mkdir

```
# mkdir dir2
# ls -l
drwxrwxrwx 1 root root 128 Jun 21 10:38 dir2
# mkdir -p dir2/dir22/dir221                  // ストール

# mkdir -p dir1/dir11/dir111
[3]sys_mkdirat: dirfd -100, path 'dir1', mode 0x1ff     // libcがdir毎に分けて
[2]sys_mkdirat: dirfd -100, path 'dir11', mode 0x1ff    // sys_mkdiratをcall
[2]sys_mkdirat: dirfd -100, path 'dir111', mode 0x1ff
# ls
bin  dev  dir1	etc  home  lib	test.txt  usr
# ls dir1
dir11
# ls dir1/dir11
dir111
# ls dir11
ls: cannot access 'dir11': No such file or directory
CurrentEL: 0x1
DAIF: Debug(1) SError(1) IRQ(1) FIQ(1)
SPSel: 0x1
SPSR_EL1: 0x0
SP: 0xffff00003bffde50
SP_EL0: 0xfffffffffce0
ELR_EL1: 0x41b510, EC: 0x0, ISS: 0x0.
FAR_EL1: 0x0
Unexpected syscall #130 (unknown)           // SYS_tkill: エラーが発生すると
kern/console.c:283: kernel panic at cpu 3.  // 発行されるようだ
```

### 何もせず0を返すsys_tkill()を実装

```
# mkdir -p dir1/dir11/dir111
# ls dir1/dir11
dir111
# ls dir11
ls: cannot access 'dir11': No such file or directory
```

## rmdir

```
# cd dir1/dir11
# ls dir111
# ls -al dir111
total 1
drwxrwxrwx 1 root root 128 Jun 21 10:31 .
drwxrwxrwx 2 root root 192 Jun 21 10:31 ..
# rmdir dir111
rmdir: failed to remove 'dir111': Not a directory
Hangup
```

### sys_unlinkat()でrmdirを行うAT_REMOVEDIRが実装されていない

- unlinkat()にAT_REMOVEDIRを実装

```
# mkdir dir1
# cd dir1
# touch test1
touch: setting times of 'test1': Invalid argument
Hangup
# ls -l
total 1
-rw-rw-rw- 1 root root 0 Jun 21 10:31 test1
# cd ..
# rmdir dir1
rmdir: failed to remove 'dir1': Operation not permitted
Hangup
# rm dir1/test1
# rmdir dir1
# ls
bin  dev  etc  home  lib  test.txt  usr
# mkdir -p dir1/dir11/dir111
# ls -l dir1
total 1
drwxrwxrwx 2 root root 192 Jun 21 10:31 dir11
# ls -l dir1/dir11
total 1
drwxrwxrwx 1 root root 128 Jun 21 10:31 dir111
# ls -l dir1/dir11/dir111
total 0
# rmdir -p dir1/dir11/dir111
# ls -l dir1
ls: cannot access 'dir1': No such file or directory
```

## touch

```
# touch test1
touch: setting times of 'test1': Invalid argument
Hangup
```
