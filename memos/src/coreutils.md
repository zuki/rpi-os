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

## uname

```
# uname
xv6
# uname -a
xv6 mini 1.0.1 2022-06-26 (musl) AArch64 Elf
```

## pwd

```
# pwd
/
# cd usr/bin
# pwd
/usr/bin
```

## wc

```
# wc test.txt
11 11 94 test.txt
# wc -l test.txt
11 test.txt
```

## whoami

```
# whoami
root
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

### sys_utimensat()がpathがNULLで呼び出されていた

- utimesnsat()は時刻を変更するファイルをpathで指定するはずがNULLであった
- pathをargstr()でparseしていたが、pathがNULLだとエラーになっていた。
- argstr()をNULLの場合はNULLを返すように修正
- utimesnsat()でpathがNULLの場合は処理をせず0を返すように修正

```
# touch file1
[0]sys_utimensat: dirfd: 0, path: (null), times[0]: (0, 0), times[1]: (12885035329, 12884901888), flags: 0x0
# ls -l
total 5
drwxrwxr-x 1 root root 896 Jun 30  2022 bin
drwxrwxr-x 1 root root 384 Jun 30  2022 dev
drwxrwxr-x 1 root root 256 Jun 30  2022 etc
-rw-rw-rw- 1 root root   0 Jun 21 10:31 file1
drwxrwxr-x 1 root root 192 Jun 30  2022 home
drwxrwxrwx 1 root root 128 Jun 30  2022 lib
-rwxr-xr-x 1 root root  94 Jun 30  2022 test.txt
drwxrwxr-x 1 root root 256 Jun 30  2022 usr
```

## ln

- シンボリックリンクはOK
- ハードリンクでエラー（エラーメッセージによるとtargetとlinkが逆）

```
# ln -s test.txt test_s.txt
# ls -l
total 5
drwxrwxr-x 1 root root 896 Jun 30  2022 bin
drwxrwxr-x 1 root root 384 Jun 30  2022 dev
drwxrwxr-x 1 root root 256 Jun 30  2022 etc
-rw-rw-rw- 1 root root   0 Jun 21 10:31 file1
drwxrwxr-x 1 root root 192 Jun 30  2022 home
drwxrwxrwx 1 root root 128 Jun 30  2022 lib
-rwxr-xr-x 1 root root  94 Jun 30  2022 test.txt
lrwxrwxrwx 1 root root   8 Jun 21 10:31 test_s.txt -> test.txt
drwxrwxr-x 1 root root 256 Jun 30  2022 usr
# ln test.txt test_h.txt
[3]fileopen: cant namei test_h.txt
ln: failed to create hard link 'test_h.txt' => 'test.txt': Invalid argument
[2]exit: exit: pid 11, name ln, err 1
Hangup
# ln --help
Usage: ln [OPTION]... [-T] TARGET LINK_NAME
  or:  ln [OPTION]... TARGET
  or:  ln [OPTION]... TARGET... DIRECTORY
  or:  ln [OPTION]... -t DIRECTORY TARGET...
In the 1st form, create a link to TARGET with the name LINK_NAME.
```

### ハードリンクとシンボリックリンクではシステムコールが異なり、ハードリンク用のシステムコールにバグ

- oldfd, newfdをstrfd()でパースしていたが、これだとAT_FDCWD(-100)が指定されているとEINVAL
- argint()でパースするように変更

```
# ls -l test*
-rwxr-xr-x 1 root root 94 Jun 30  2022 test.txt
# ln test.txt test_h.txt
[1]fileopen: cant namei test_h.txt                // エラー出力があるが
# ln -s test.txt test_s.txt
# ls -l test*
-rwxr-xr-x 2 root root 94 Jun 30  2022 test.txt
-rwxr-xr-x 2 root root 94 Jun 30  2022 test_h.txt   // リンクは正常にできている
lrwxrwxrwx 1 root root  8 Jun 21 10:31 test_s.txt -> test.txt
# rm test_h.txt test_s.txt
# ls -l test*
-rwxr-xr-x 1 root root 94 Jun 30  2022 test.txt
```

### fileopenエラーが出ている件

- libcがまずターゲットが存在しないかチェックしているためで、存在しないためエラーになるのは正常
- エラーメッセージはwarn()で出しているが現在のログレベルはLOG_INFOなので出力される。LOG_ERRORにすれば出力されないのでこのままとする。

```
# ls -l test*
[3]sys_openat: dirfd -100, path '.', flag 0xa4000, mode0x0
[1]sys_openat: dirfd -100, path '/etc/passwd', flag 0xa0000, mode0x1b6
[0]sys_openat: dirfd -100, path '/etc/group', flag 0xa0000, mode0x1b6
-rwxr-xr-x 1 root root 94 Jun 30  2022 test.txt
# ln test.txt test_h.txt
[0]sys_openat: dirfd -100, path 'test_h.txt', flag 0x224000, mode0x0    // linkpathの存在をチェック
[0]fileopen: cant namei test_h.txt                                      // 存在しないのでエラーは正常
[2]sys_linkat: oldfd: -100, oldpath: test.txt, newfd: -100, newpath: test_h.txt, flags: 1024
[1]sys_linkat: oldfd: -100, oldpath: test.txt, newfd: -100, newpath: test_h.txt, flags: 0
# ls -l test*
[2]sys_openat: dirfd -100, path '.', flag 0xa4000, mode0x0
[1]sys_openat: dirfd -100, path '/etc/passwd', flag 0xa0000, mode0x1b6
[0]sys_openat: dirfd -100, path '/etc/group', flag 0xa0000, mode0x1b6
-rwxr-xr-x 2 root root 94 Jun 30  2022 test.txt
-rwxr-xr-x 2 root root 94 Jun 30  2022 test_h.txt
# ln -s test.txt test_s.txt
[3]sys_symlinkat: fd: -100, target: test.txt, path: test_s.txt
# ls -l test*
[3]sys_openat: dirfd -100, path '.', flag 0xa4000, mode0x0
[3]sys_openat: dirfd -100, path '/etc/passwd', flag 0xa0000, mode0x1b6
[3]sys_openat: dirfd -100, path '/etc/group', flag 0xa0000, mode0x1b6
-rwxr-xr-x 2 root root 94 Jun 30  2022 test.txt
-rwxr-xr-x 2 root root 94 Jun 30  2022 test_h.txt
lrwxrwxrwx 1 root root  8 Jun 21 10:32 test_s.txt -> test.txt
# cat test_s.txt
[0]sys_openat: dirfd -100, path 'test_s.txt', flag 0x20000, mode0x0
123
111
111
123
999
あいうえお
あいうえお
999
かきくけこ
12345
あいうえお
# cat test_h.txt
[0]sys_openat: dirfd -100, path 'test_h.txt', flag 0x20000, mode0x0
123
111
111
123
999
あいうえお
あいうえお
999
かきくけこ
12345
あいうえお
# cat test.txt
[2]sys_openat: dirfd -100, path 'test.txt', flag 0x20000, mode0x0
123
111
111
123
999
あいうえお
あいうえお
999
かきくけこ
12345
あいうえお
# rm test_s.txt test_h.txt      // 複数ファイルを指定したrmも成功している
# ls -l test*
[3]sys_openat: dirfd -100, path '.', flag 0xa4000, mode0x0
[2]sys_openat: dirfd -100, path '/etc/passwd', flag 0xa0000, mode0x1b6
[0]sys_openat: dirfd -100, path '/etc/group', flag 0xa0000, mode0x1b6
-rwxr-xr-x 1 root root 94 Jun 30  2022 test.txt
```

## mv

```
# mv mvtest mvtest2
CurrentEL: 0x1
DAIF: Debug(1) SError(1) IRQ(1) FIQ(1)
SPSel: 0x1
SPSR_EL1: 0x20000000
SP: 0xffff00003bee3e50
SP_EL0: 0xfffffffffc70
ELR_EL1: 0x42ca94, EC: 0x0, ISS: 0x0.
FAR_EL1: 0x0
Unexpected syscall #38 (unknown)              // 38: SYS_renameat
kern/console.c:283: kernel panic at cpu 0.
```

### SYS_renameat2()は実装していたが、SYS_renameat()は実装していなかった

- SYS_renameat()を実装

```
# mv mvtest mvtest2                             // file -> fileはOK
# cat mvtest2
123
abc
# ls -l mvtest*
-rw-rw-rw- 1 root root 8 Jun 21 10:31 mvtest2
# mkdir dir1
# mv mvtest2 dir1                               // file -> dirはNG
# cat dir1/mvtest2
[0]fileopen: cant namei dir1/mvtest2
cat: dir1/mvtest2: No such file or directory
[1]exit: exit: pid 16, name cat, err 1
Hangup
# ls dir1
# ls -l
total 5
drwxrwxr-x 1 root root 896 Jun 30  2022 bin
drwxrwxr-x 1 root root 384 Jun 30  2022 dev
drwxrwxrwx 1 root root 128 Jun 21 10:32 dir1
drwxrwxr-x 1 root root 256 Jun 30  2022 etc
drwxrwxr-x 1 root root 192 Jun 30  2022 home
drwxrwxrwx 1 root root 128 Jun 30  2022 lib
-rw-rw-rw- 1 root root   8 Jun 21 10:31 mvtest2   // コピーされていない
-rwxr-xr-x 1 root root  94 Jun 30  2022 test.txt
drwxrwxr-x 1 root root 256 Jun 30  2022 usr
# mv mvtest2 dir1/                                // '/'をつけても結果は同じ
# cat dir1/mvtest2
[1]fileopen: cant namei dir1/mvtest2
cat: dir1/mvtest2: No such file or directory
[2]exit: exit: pid 20, name cat, err 1
Hangup
# ls -l
drwxrwxrwx 1 root root 128 Jun 21 10:32 dir1
rw-rw-rw- 1 root root   8 Jun 21 10:31 mvtest2
# ls -al dir1
total 2
drwxrwxrwx 1 root root  128 Jun 21 10:32 .
drwxrwxr-x 2 root root 1024 Jun 30  2022 ..
```

- filerename()のdebug
- 処理がおかしい

```
(gdb) p path1
$1 = 0xfffffffffff6 "mvtest"
(gdb) p path2
$2 = 0x600000001230 "dir1/mvtest"
(gdb) p dp1->inum                         // dp1: '/'
$6 = 1
(gdb) p ip1->inum                         // ip1: 'mvtest'
$3 = 142
(gdb) p ip2
$7 = <optimized out>
(gdb) p name2
$8 = "mvtest\000 .... "
(gdb) p ip1->type                         // ip1はtype=2のはずだが
$9 = 1
(gdb) n
805	            if ((error = rename(dp1, name1, name2)) < 0) {
(gdb) n
843	    end_op();
(gdb) n
844	    return error;
```

```
# mv mvtest dir1/
[0]namex: inum: 111, type: 1
[3]namex: inum: 111, type: 1
[0]namex: inum: 141, type: 2      // dir1のtypeが2->1へ
[0]namex: inum: 141, type: 1
[1]namex: inum: 140, type: 1      // mvtestのtype=1
```

- direntにtypeがないため、dirlookup()でiget()した時にtypeがわからない
- direntにtypeを追加

```
# mv mvtest dir1
[3]filerename: path1: mvtest, dp1: 1, ip1: 140, name1: mvtest
[3]filerename: path2: dir1/mvtest, dp2: 141, ip2: -1, name2: mvtest
[3]rename: writei
[3]filerename: rename3 failed
# mv mvtest mvtest2
[3]filerename: path1: mvtest, dp1: 1, ip1: 140, name1: mvtest
[3]filerename: path2: mvtest2, dp2: 1, ip2: -1, name2: mvtest2  // ストール
# mv dir1 dir2
[1]filerename: path1: dir1, dp1: 1, ip1: 141, name1: dir1
[1]filerename: path2: dir2, dp2: 1, ip2: -1, name2: dir2    // ストール
```

- rename()でdp1==dp2の場合、ilock(dp2)をしない

```
# mv mvtest mvtest2
[2]filerename: path1: mvtest, dp1: 1, ip1: 140, name1: mvtest
[2]filerename: path2: mvtest2, dp2: 1, ip2: -1, name2: mvtest2
# ls
bin  dev  etc  home  lib  mvtest  mvtest2  test.txt  usr    // cpになってしまった
# ls -l
-rw-rw-rw- 1 root root   8 Jun 21 10:31 mvtest
-rw-rw-rw- 1 root root   8 Jun 21 10:31 mvtest2
# cat mvtest
abc
123
# cat mvtest2
abc
123
# mkdir dir1
# mv dir1 dir2
[1]filerename: path1: dir1, dp1: 1, ip1: 141, name1: dir1
[1]filerename: path2: dir2, dp2: 1, ip2: -1, name2: dir2        // dir2を作成して
[1]filerename: path1: dir1, dp1: 1, ip1: 141, name1: dir1
[1]filerename: path2: dir2/dir1, dp2: 141, ip2: -1, name2: dir1 // その下に置こうとしている。ストール
```

### filerename()のロジックを修正

```
# ls
bin  dev  etc  home  lib  test.txt  usr
# cat > mvtest
c
123
abc
# ls -il mvtest
140 -rw-rw-rw- 1 root root 12 Jun 21 10:31 mvtest
# mv mvtest mvtest2
[0]filerename: path1: mvtest, dp1: 1, ip1: 140, name1: mvtest
[0]filerename: path2: mvtest2, dp2: 1, ip2: -1, name2: mvtest2
# ls -il
140 -rw-rw-rw- 1 root root  12 Jun 21 10:31 mvtest2
# mkdir dir1
# ls -il
141 drwxrwxrwx 1 root root 128 Jun 21 10:32 dir1
140 -rw-rw-rw- 1 root root  12 Jun 21 10:31 mvtest2
# mv dir1 dir2
# ls -il
total 5
141 drwxrwxrwx 1 root root 128 Jun 21 10:32 dir2
140 -rw-rw-rw- 1 root root  12 Jun 21 10:31 mvtest2
# mv mvtest2 dir2
# ls -il
total 5
  2 drwxrwxr-x 1 root root 896 Jul  1  2022 bin
  3 drwxrwxr-x 1 root root 384 Jul  1  2022 dev
141 drwxrwxrwx 1 root root 192 Jun 21 10:32 dir2
  8 drwxrwxr-x 1 root root 256 Jul  1  2022 etc
 10 drwxrwxr-x 1 root root 192 Jul  1  2022 home
  9 drwxrwxrwx 1 root root 128 Jul  1  2022 lib
 28 -rwxr-xr-x 1 root root  94 Jul  1  2022 test.txt
 12 drwxrwxr-x 1 root root 256 Jul  1  2022 usr
# ls -il dir2
total 1
140 -rw-rw-rw- 0 root root 12 Jun 21 10:31 mvtest2
```
