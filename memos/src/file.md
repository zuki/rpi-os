# 08: ファイル情報を追加

## ファイルとinodeに以下のフィールドを追加および一部変更

- `struct file`にflagsを追加
- `struct inode`にmode, atime, mtime, ctimeを追加
- `struct dinode`にも同じフィールドを追加
- `struct dirent`のサイズを16バイトから64バイトに変更
    - inumを4バイトに、nameを60バイトに拡張

## ファイル関係を中心にシステムコールを追加

- sys_dup3
- sys_fcntl
- sys_symlinkat
- sys_linkat
- sys_getdents64
- sys_lseek
- sys_readv
- sys_readlinkat
- sys_fsync
- sys_utimensat
- sys_fadvise64

## v6ファイルシステムを変更

- FSサイズ、inode数、ファイル数などを大きくした
- `/bin`, `/dev`, `/etc`, `/home`, `/usr/local`を作成
- `usr/src/mkfs`と`tools/dump`を修正

## 変更分確認のため`usr/src/ls`を編集

```
$ /bin/ls
40775    1   512  6 24 16:42 .
40775    1   512  6 24 16:42 ..
40775    2   896  6 24 16:42 bin
40775    3   384  6 24 16:42 dev
40775    8   128  6 24 16:42 etc
40777    9   128  6 24 16:42 lib
40775   10   192  6 24 16:42 home
40775   12   192  6 24 16:42 usre
$ /bin/ls /bin
40775    2   896  6 24 16:42 .
40775    1   512  6 24 16:42 ..
100755   15 38568  6 24 16:42 cat
100755   16 22448  6 24 16:42 init
100755   17 39816  6 24 16:42 bigtest
100755   18 39480  6 24 16:42 echoest
100755   19 49048  6 24 16:42 mkfsest
100755   20 48552  6 24 16:42 dateest
100755   21 54056  6 24 16:42 shteest
100755   22 22104  6 24 16:42 sigtest3
100755   23 48640  6 24 16:42 sigtest2
100755   24 17744  6 24 16:42 utestst2
100755   25 45040  6 24 16:42 sigtest2
100755   26 52504  6 24 16:42 lsgtest2
$ /bin/ls /dev
40775    3   384  6 24 16:42 .
40775    1   512  6 24 16:42 ..
60666    4     0  6 24 16:42 sdc1
60666    5     0  6 24 16:42 sdc2
60666    6     0  6 24 16:42 sdc3
20666    7     0  6 21 10:31 tty3
$ /bin/ls /etc
40775    8   128  6 24 16:42 .
40775    1   512  6 24 16:42 ..
$ /bin/ls /lib
40777    9   128  6 24 16:42 .
40775    1   512  6 24 16:42 ..
$ /bin/ls /home
40775   10   192  6 24 16:42 .
40775    1   512  6 24 16:42 ..
40775   11   128  6 24 16:42 vagrant
$ /bin/ls /usr
40775   12   192  6 24 16:42 .
40775    1   512  6 24 16:42 ..
40775   13   192  6 24 16:42 local
$ /bin/ls /usr/local
40775   13   192  6 24 16:42 .
40775   12   192  6 24 16:42 ..
40775   14   128  6 24 16:42 bin
$ /bin/ls /usr/local/bin
40775   14   128  6 24 16:42 .
40775   13   192  6 24 16:42 ..
```

# includeファイルについて

- カーネルソースでlibcとincにあるincludeファイルを混在させると重複定義が頻発する
- libcにある必要なファイルを`inc/linux`にコピーし`inc`配下のファイルしか使わないようにした

# 変更履歴

```
$ git status
On branch mac
	modified:   Makefile
	modified:   inc/arm.h
	modified:   inc/buf.h
	modified:   inc/clock.h
	modified:   inc/console.h
	modified:   inc/debug.h
	modified:   inc/file.h
	modified:   inc/fs.h
	new file:   inc/linux/elf.h
	new file:   inc/linux/errno.h
	new file:   inc/linux/fcntl.h
	new file:   inc/linux/ioctl.h
	new file:   inc/linux/mman.h
	new file:   inc/linux/ppoll.h
	renamed:    inc/signal.h -> inc/linux/signal.h
	new file:   inc/linux/stat.h
	new file:   inc/linux/syscall.h
	new file:   inc/linux/termios.h
	renamed:    inc/time.h -> inc/linux/time.h
	modified:   inc/list.h
	modified:   inc/memlayout.h
	modified:   inc/mm.h
	new file:   inc/mmap.h
	new file:   inc/pipe.h
	modified:   inc/proc.h
	modified:   inc/rtc.h
	modified:   inc/string.h
	modified:   inc/syscall1.h
	modified:   inc/trap.h
	modified:   inc/types.h
	modified:   inc/vm.h
	modified:   kern/clock.c
	modified:   kern/console.c
	modified:   kern/exec.c
	modified:   kern/file.c
	modified:   kern/fs.c
	modified:   kern/icode.S
	modified:   kern/kpgdir.c
	modified:   kern/kpgdir.pl
	modified:   kern/main.c
	modified:   kern/pipe.c
	modified:   kern/proc.c
	modified:   kern/rtc.c
	modified:   kern/signal.c
	modified:   kern/sigret_syscall.S
	modified:   kern/syscall.c
	modified:   kern/sysfile.c
	modified:   kern/sysproc.c
	modified:   kern/timer.c
	modified:   kern/trap.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/file.md
	modified:   mksd.mk
	modified:   tools/dump.c
	modified:   usr/Makefile
	new file:   usr/inc/fs.h
	new file:   usr/inc/param.h
	new file:   usr/inc/types.h
	modified:   usr/src/init/main.c
	modified:   usr/src/ls/main.c
	modified:   usr/src/mkfs/main.c
	modified:   usr/src/sh/main.c
```
