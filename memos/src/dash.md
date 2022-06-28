# 13: dashを導入

https://git.kernel.org/pub/scm/utils/dash/dash.git/ からdash-0.5.11.5.tar.gzをダウンロード

```
$ tar xf dash-0.5.11.5.tar.gz
$ cd dash-0.5.11.5
$ sh autogen.sh
$ CC=/Users/dspace/musl/bin/musl-gcc ./configure --host=aarch64-elf --enable-static CFLAGS="-std=gnu99 -O3 -MMD -MP -static -fno-plt -fno-pic -fpie -z max-page-size=4096 -Isrc"
$ make
$ file src/dash
src/dash: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, with debug_info, not stripped
```

## xv6に導入

```
$ mv src/dash ../usr/bin/
$ vi usr/inc/usrbins.h
$ vi usr/src/init/main.c
$ cd $XV6HOME
$ rm -rf obj/usr
$ make
$ make qemu
...
[0]iinit: sb: size 800000 nblocks 799455 ninodes 1024 nlog 90 logstart 2 inodestart 92 bmapstart 349
init: starting sh
init: starting sh   // 以下、繰り返し
```

## dash立ち上げに失敗

```
init: starting sh
pid=6, wpid=-1
init: starting sh
pid=7, wpid=-1
init: starting sh
pid=8, wpid=-1
init: starting sh
pid=9, wpid=-1
```

#### sys_wait4()の第4パラメタ `stryct rusage *rusage`がNULLの場合、argptr()が`-1`を返していたため、ここで`EINVAL`エラーとなっていた。

- 他にも構造体へのポインタである引数がNULLを持つケースは大きいため、agrptr()でポインタがNULLの場合は無条件で成功とした。

```
[0]execve: path=/bin/init, argv=none, envp=none
init: starting sh                                                       # L1
[0]wait4: pid=-1, status=0xffffffffff9c, options=0, ru=0x0              # L13
[2]execve: path=/usr/bin/dash, argv=dash, envp=TEST_ENV=FROM_INIT       # L9
[3]wait4: pid=6, state=5, options=0
[3]wait4: return pid=6
[3]trap: x0=6
[3]trap: check_pending_signal ok
pid=6, wpid=6, status=1                                                 # L15
init: starting sh
```

#### `path=/usr/bin/dash`がexit(1)で終了している

```c
// usr/src/init/main.c
 1: printf("init: starting sh\n");
 2: pid = fork();
 3: if (pid < 0) {
 4:     printf("init: fork failed\n");
 5:     exit(1);
 6: }
 7: if (pid == 0) {
 8:     //execve("/bin/sh", argv, envp);
 9:     execve("/usr/bin/dash", argv, envp);
10:     printf("init: exec sh failed\n");
11:     exit(1);
12: }
13: while ((wpid = wait(&status)) >= 0 && wpid != pid)
14:     printf("zombie!\n");
15: printf("pid=%d, wpid=%d, status=%d\n", pid, wpid, status);
```

## `/bin/sh`は正常に動く

- /bin/shのプロンプトを`#`に変更(dashと区別するため)
- dashは動かない
- coreutilsコマンドも動かないものが多い

```
# /usr/bin/dash
[2]exit: exit: pid 7, name dash, err 1
# /bin/ls /bin
drwxrwxr-x    2 root wheel   896  6 27 16:05 .
drwxrwxr-x    1 root wheel   512  6 27 16:05 ..
-rwxr-xr-x   16 root wheel 38568  6 27 16:05 cat
-rwxr-xr-x   17 root wheel 44736  6 27 16:05 init
...
# /usr/bin/echo abc
abc
# /usr/bin/echo abc > test
open test failed
# /bin/ls /usr/bin | /usr/bin/head
head: error reading 'standard input': Invalid argument
head: -: Operation not permitted
# /usr/bin/ls
ls: memory exhausted
```

## shでdashを動かした場合のシステムコール

- sys_mmapのちゃんとした実装が必要

```
# /usr/bin/dash
[2]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_clone called
[0]syscall1: sys_gettid called
[2]syscall1: sys_rt_sigprocmask called
[0]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_rt_sigprocmask called
[0]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_wait4 called
[3]syscall1: sys_execve called
[3]syscall1: sys_gettid called
[3]syscall1: sys_getpid called
[3]syscall1: sys_rt_sigprocmask called
[3]syscall1: sys_rt_sigaction called
[3]syscall1: sys_geteuid called
[3]syscall1: sys_brk called
[3]syscall1: sys_brk called
[3]syscall1: sys_mmap called
[3]syscall1: sys_mmap called
[1]syscall1: sys_mmap called                // 以下、39回 [1]sys_mmap called
...
[1]exit: exit: pid 7, name dash, err 1
[1]syscall1: sys_writev called

#
```

- sys_brk, sys_mmapの呼び出し

```
# /usr/bin/dash
[0]sys_brk: name dash: 0x42f9e0 to 0x0
[0]sys_brk: name dash: 0x42f9e0 to 0x432000
[0]sys_mmap: addr=0x430000, len=0x4096, prot=0x0, flags=0x32, off=0x0
[0]sys_mmap: addr=0x0, len=0x4096, prot=0x3, flags=0x22, off=0x0
```

```c
// libc/src/malloc/mallong/malloc.c#L247
p = mmap(0, needed, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
if (p==MAP_FAILED) {
    free_meta(m);
    return 0;
}

// kern/sysproc.c#L105
} else {
    if (prot != (PROT_READ | PROT_WRITE)) {
        warn("non-rw unimplemented");
        return -EPERM;
    }
    //panic("unimplemented. ");
    return -EPERM;                  // まったく実装していなかった
}
```

## gdbが動かなかったのはmacのセキュリティ設定のためだった

[GDB Wiki: PermissionsDarwin](https://sourceware.org/gdb/wiki/PermissionsDarwin)の指示通りに処理したところ動くようになった。

```
$ make gdb
gdb -n -x .gdbinit
GNU gdb (GDB) 11.2
...

The target architecture is set to "aarch64".
0x0000000000000000 in ?? ()
Breakpoint 1 at 0xffff00000008dacc: proc.c:377. (2 locations)
(gdb) c
Continuing.
[Switching to Thread 1.3]

Thread 3 hit Breakpoint 1, wait4 (pid=-1, status=0xffffffffff9c, options=<optimized out>, ru=0x0) at kern/proc.c:378
378	        LIST_FOREACH_ENTRY_SAFE(p, np, que, clink) {
(gdb) n
379	            if (p->parent != cp) continue;
(gdb)
```
