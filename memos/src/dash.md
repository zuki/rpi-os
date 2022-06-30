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

## sys_mmapを実装

```
# /usr/bin/dash
[3]sys_brk: name dash: 0x42f9e0 to 0x0                                          // [1]
[1]sys_brk: name dash: 0x42f9e0 to 0x432000                                     // [2] ここで0x432000までmappingしているので
[1]sys_mmap: addr=0x430000, length=0x4096, prot=0x0, flags=0x32, offset=0x0     // [3] sys_brk(432000)して、またsys_mmap(432000)するのは何故?(PROT_NONEで未使用領域にするためだった)
[1]uvm_map: remap: p=0x430000, *pte=0x3bfbd647                                  // [4] 0x430000がremapなのは当然

[1]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0          // [5]
[1]uvm_map: remap: p=0x431000, *pte=0x3bfc1647

[1]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0          // [6]
[1]sys_munmap: addr: 0x432000, length: 0x1000                                   // [7]
[1]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0          // [8]
[1]sys_munmap: addr: 0x600000000000, length: 0x1000                             // [9]

pid 7, exitshell(2)
[0]exit: exit: pid 7, name dash, err 2
```

## libc/src/malloc/malloc.c

```c
struct meta *alloc_meta(void)
{
	struct meta *m;
	unsigned char *p;
	if (!ctx.init_done) {
#ifndef PAGESIZE
		ctx.pagesize = get_page_size();
#endif
		ctx.secret = get_random_secret();
		ctx.init_done = 1;
	}
	size_t pagesize = PGSZ;
	if (pagesize < 4096) pagesize = 4096;
	if ((m = dequeue_head(&ctx.free_meta_head))) return m;
	if (!ctx.avail_meta_count) {
		int need_unprotect = 1;
		if (!ctx.avail_meta_area_count && ctx.brk!=-1) {
			uintptr_t new = ctx.brk + pagesize;
			int need_guard = 0;
			if (!ctx.brk) {
				need_guard = 1;
				ctx.brk = brk(0);                           // [1] ctx.brk = 0x42f9e0
				ctx.brk += -ctx.brk & (pagesize-1);         //     ctx.brk = 0x430000
				new = ctx.brk + 2*pagesize;                 //     new     = 0x432000
			}
			if (brk(new) != new) {                          // [2] brk(new) = 0x43200
				ctx.brk = -1;                               //     ここで0x430000-0x432000までmapping
			} else {                                        // こちらが実行される
				if (need_guard) mmap((void *)ctx.brk, pagesize, // [3] 0x430000から1ページPROT_NONE
					PROT_NONE, MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1, 0);  // PROT_NONEが未実装なのでmappingしてremap
				ctx.brk = new;                                      // ctx.brk = 0x43200
				ctx.avail_meta_areas = (void *)(new - pagesize);    // avail_metaareas: 0x431000 - 0x432000
				ctx.avail_meta_area_count = pagesize>>12;           // avail_meta_area_count = 1
				need_unprotect = 0;
			}
		}
		if (!ctx.avail_meta_area_count) {                   // 該当せず
			size_t n = 2UL << ctx.meta_alloc_shift;
			p = mmap(0, n*pagesize, PROT_NONE,
				MAP_PRIVATE|MAP_ANON, -1, 0);
			if (p==MAP_FAILED) return 0;
			ctx.avail_meta_areas = p + pagesize;
			ctx.avail_meta_area_count = (n-1)*(pagesize>>12);
			ctx.meta_alloc_shift++;
		}
		p = ctx.avail_meta_areas;                               // p = 0x431000
		if ((uintptr_t)p & (pagesize-1)) need_unprotect = 0;    // 該当せず
		if (need_unprotect)                                     // 該当せず
			if (mprotect(p, pagesize, PROT_READ|PROT_WRITE)
			    && errno != ENOSYS)
				return 0;
		ctx.avail_meta_area_count--;                            // avail_meta_area_count = 0
		ctx.avail_meta_areas = p + 4096;                        // avail_meta_areas = 0x432000 (未mapping)
		if (ctx.meta_area_tail) {
			ctx.meta_area_tail->next = (void *)p;
		} else {
			ctx.meta_area_head = (void *)p;
		}
		ctx.meta_area_tail = (void *)p;
		ctx.meta_area_tail->check = ctx.secret;
		ctx.avail_meta_count = ctx.meta_area_tail->nslots
			= (4096-sizeof(struct meta_area))/sizeof *m;
		ctx.avail_meta = ctx.meta_area_tail->slots;
	}
	ctx.avail_meta_count--;
	m = ctx.avail_meta++;
	m->prev = m->next = 0;
	return m;
}
```

### sys_mmapで`PROT_NONE`を実装

```
# /usr/bin/dash
[0]sys_brk: name dash: 0x42f9e0 to 0x0
[0]sys_brk: name dash: 0x42f9e0 to 0x432000
[0]sys_mmap: addr=0x430000, length=0x4096, prot=0x0, flags=0x32, offset=0x0
[0]sys_mmap: return 0x430000
[0]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0
[0]sys_mmap: return 0x600000000000      // ここでストール
```

```
# /usr/bin/dash
[2]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_clone called
[3]syscall1: sys_gettid called
[2]syscall1: sys_rt_sigprocmask called
[3]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_rt_sigprocmask called
[3]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_wait4 called
[3]syscall1: sys_execve called
[2]syscall1: sys_gettid called
[2]syscall1: sys_getpid called
[2]syscall1: sys_rt_sigprocmask called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_geteuid called
[2]syscall1: sys_brk called
[2]sys_brk: name dash: 0x42f9e0 to 0x0
[2]syscall1: sys_brk called
[2]sys_brk: name dash: 0x42f9e0 to 0x432000
[2]syscall1: sys_mmap called
[2]sys_mmap: addr=0x430000, length=0x4096, prot=0x0, flags=0x32, offset=0x0
[0]syscall1: sys_mmap called
[0]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0
[2]syscall1: sys_getppid called
[2]syscall1: sys_getcwd called          // これが問題か
```

### sys_getcwd

- cwdがルートディレクトリの場合、cwdとdpが同じinodeを指すのでacquiresleep()でデッドロックになっていた。

```
/usr/bin/dash
[3]syscall1: sys_brk called
[3]sys_brk: name dash: 0x42f9e0 to 0x0
[3]syscall1: sys_brk called
[3]sys_brk: name dash: 0x42f9e0 to 0x432000
[3]syscall1: sys_mmap called
[3]sys_mmap: addr=0x430000, length=0x4096, prot=0x0, flags=0x32, offset=0x0
[3]sys_mmap: return 0x430000
[3]syscall1: sys_mmap called
[3]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0
[3]sys_mmap: return 0x600000000000
[3]syscall1: sys_getppid called
[3]syscall1: sys_getcwd called
[3]syscall1: sys_ioctl called
[3]syscall1: sys_ioctl called
[3]syscall1: sys_rt_sigaction called
[3]syscall1: sys_rt_sigaction called
[3]syscall1: sys_rt_sigaction called
[3]syscall1: sys_rt_sigaction called
[3]syscall1: sys_rt_sigaction called
[3]syscall1: sys_rt_sigaction called
[3]syscall1: sys_openat called
[3]syscall1: sys_fcntl called
[3]syscall1: sys_close called
[3]syscall1: sys_fcntl called
[3]syscall1: sys_ioctl called			// 以後、sys_iocntl, sys_getpgid, sys_killの
[3]syscall1: sys_getpgid called			// 組で無限ループ
[3]syscall1: sys_kill called
```

### sys_ioctlの問題だった

- TIOCSPGRP, TIOCGPGRPを実装していなかったため

```
/usr/bin/dash
[1]syscall1: sys_brk called
[1]sys_brk: name dash: 0x42f9e0 to 0x0
[1]syscall1: sys_brk called
[1]sys_brk: name dash: 0x42f9e0 to 0x432000
[1]syscall1: sys_mmap called
[1]sys_mmap: addr=0x430000, length=0x4096, prot=0x0, flags=0x32, offset=0x0
[1]sys_mmap: return 0x430000
[0]syscall1: sys_mmap called
[0]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0
[0]sys_mmap: return 0x600000000000
[2]syscall1: sys_getppid called
[2]syscall1: sys_getcwd called
[2]syscall1: sys_ioctl called
[2]sys_ioctl: fd: 0, req: 0x5413, f->type: 3
[2]syscall1: sys_ioctl called
[2]sys_ioctl: fd: 1, req: 0x5413, f->type: 3
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_openat called
[2]syscall1: sys_fcntl called
[2]syscall1: sys_close called
[2]syscall1: sys_fcntl called
[2]syscall1: sys_ioctl called
[2]sys_ioctl: fd: 10, req: 0x540f, f->type: 3
[2]syscall1: sys_getpgid called
[2]sys_getpgid: pid 0, return 1
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_rt_sigaction called
[2]syscall1: sys_setpgid called
[2]syscall1: sys_ioctl called
[2]sys_ioctl: fd: 10, req: 0x5410, f->type: 3
[2]syscall1: sys_getuid called
[2]syscall1: sys_geteuid called
[2]syscall1: sys_getgid called
[2]syscall1: sys_getegid called
[2]syscall1: sys_write called
[2]syscall1: sys_read called		// ここでストール
```

### 何点か修正することでdashのプロンプトがでる

- kill()の`if (pid == 0 || pid < -1)`の場合を処理
- in_user()でmmap_regiomを考慮
- sys_ioctl()で変数pgid, pgid_pをswitch()の外で定義

```
$ /usr/bin/dash								// /bin/shのプロンプトを戻した
[1]sys_brk: name dash: 0x42f3c8 to 0x0
[1]sys_brk: name dash: 0x42f3c8 to 0x432000
[1]sys_mmap: addr=0x430000, length=0x4096, prot=0x0, flags=0x32, offset=0x0
[1]sys_mmap: return 0x430000
[3]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0
[3]sys_mmap: return 0x600000000000
[0]sys_ioctl: fd: 0, req: 0x5413, f->type: 3
[0]sys_ioctl: fd: 1, req: 0x5413, f->type: 3
[0]sys_ioctl: fd: 10, req: 0x540f, f->type: 3
[0]sys_ioctl: TIOCGPGRP: pid 7, pgid 1, pgid_p 1
[0]sys_getpgid: pid 0, return 1
[0]sys_ioctl: fd: 10, req: 0x5410, f->type: 3
[0]sys_ioctl: TIOCSPGRP: pid 7, pgid -500 -> -500	// 指定のpgidの値がおかしい

# /usr/bin/ls								// rootなのでdashのプロンプトは`#`
kern/vm.c:78: assertion failed.				// uvm_copy()内のassert					dash step 1
```

### ioctl()はdash/src/jobs.c#setjobctl()内で呼び出している

```c
		fd = savefd(fd, ofd);
		do { /* while we are in the background */
            printf("call ioctlr GETPGRP\n");	// [1]
			if ((pgrp = tcgetpgrp(fd)) < 0) {	// [2]
out:
				sh_warnx("can't access tty; job control turned off");
				mflag = on = 0;
				goto close;
			}
            printf("pgrp: %d\n", pgrp);			// [3]
			if (pgrp == getpgrp()) {			// [4]
                printf("call getpgid: pgid=%d\n", pgrp);	// [5]
                break;
            }

			killpg(0, SIGTTIN);
		} while (1);
		initialpgrp = pgrp;

		setsignal(SIGTSTP);
		setsignal(SIGTTOU);
		setsignal(SIGTTIN);
		pgrp = rootpid;
		setpgid(0, pgrp);
        printf("call ioctlr GETSGRP: pgid=%d\n", pgrp);	// [6]
		xtcsetpgrp(fd, pgrp);
```
```
call ioctlr GETPGRP										// [1]
[3]sys_ioctl: fd: 10, req: 0x540f, f->type: 3			// [2]
[3]sys_ioctl: TIOCGPGRP: pid 7, pgid 1, pgid_p 1
pgrp: 1													// [3]
[2]sys_getpgid: pid 0, return 1							// [4]
call getpgid: pgid=1									// [5]
call ioctlr GETSGRP: pgid=7								// [6] pgid=7で呼び出している
[2]sys_ioctl: fd: 10, req: 0x5410, f->type: 3
[2]sys_ioctl: TIOCSPGRP: pid 7, pgid -516 -> -516		// 何故、-516
```

### ioctl()のTIOCSPGRPの第3パラメタもポインタだった

```c
int tcsetpgrp(int fd, pid_t pgrp)
{
    int pgrp_int = pgrp;
    return ioctl(fd, TIOCSPGRP, &pgrp_int);
}
```

- sys_ioctl()を修正して正しい値が設定された

```
[3]sys_ioctl: TIOCSPGRP: pid 7, pgid 7 -> 7
```

## `kern/vm.c:78: assertion failed.	`の件

```c
// kern/vm.c#uvm_copy()
	if (pgt3[i3] & PTE_VALID) {
		assert(pgt3[i3] & PTE_PAGE);
		assert(pgt3[i3] & PTE_USER);  	//	PROT_NONEの場合はPTE_USERは0なのでassert失敗
		assert(pgt3[i3] & PTE_NORMAL);
```

- 当該行をコメントアウト

```
# /bin/ls
[1]uvm_map: remap: p=0x600000000000, *pte=0x3bf2d647						// これが原因でforkに失敗

/usr/bin/dash: 1: Cannot fork
[2]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0
[2]sys_mmap: return 0x600000001000
[1]sys_munmap: addr: 0x600000001000, length: 0x1000
```

### fork時の親から子へのマッピングテーブルのコピー方法が変わっていた

- mappingはmmap_region文を含めてuvm_copy()でコピーされる
- copy_mmap_list()でmmap_region分のmmpingをしたのでremapとなった
- copy_mmap_list()ではmmap_regionのコピーだけでmappingしないように変更

```
# /bin/ls
CurrentEL: 0x1
DAIF: Debug(1) SError(1) IRQ(1) FIQ(1)		// copy on writeの処理をtrapで
SPSel: 0x1									// していないためと思わ割れる
SPSR_EL1: 0x600003c5
SP: 0xffff00003bf92d50
SP_EL0: 0xfffffffffb40
ELR_EL1: 0xffff000000082c7c, EC: 0x25, ISS: 0x4.
FAR_EL1: 0x1000000000000
irq of type 4 unimplemented.									// dash step 2
```

### type 4 irqを実装

- trap.cでEC_DABORT, EC_DABORT2の場合のハンドラを実装

```
$ /usr/bin/dash
# /bin/ls				// コマンド入力でハング  step 3
```

- syscallをプリント

```
[1]syscall1: sys_rt_sigprocmask called
[1]syscall1: sys_clone called
[2]syscall1: sys_rt_sigprocmask called
[0]syscall1: sys_getpid called
[2]syscall1: sys_setpgid called
[0]syscall1: sys_setpgid called
[0]syscall1: sys_ioctl called
[2]syscall1: sys_wait4 called
[0]syscall1: sys_rt_sigaction called
[0]syscall1: sys_rt_sigaction called
[0]syscall1: sys_rt_sigaction called
[0]syscall1: sys_rt_sigaction called
[0]syscall1: sys_rt_sigaction called
[0]syscall1: sys_rt_sigprocmask called
[0]syscall1: sys_execve called				// execveで問題発生	dash step 3
```

- exec.c#L144 dccivac() でストール
- L144をコメントアウトするとexec.c#205でストール

### fetchstr()を修正

- addrがmmap_regionにある場合を考慮するよう変更
- dashでコマンド実行成功

```
$ /usr/bin/dash
[3]execve: argv[0] = '/usr/bin/dash', len: 13
[3]execve: envp[0] = 'TEST_ENV=FROM_INIT', len: 18
[3]execve: envp[1] = 'TZ=JST-9', len: 8
# /bin/ls /
[2]execve: argv[0] = '/bin/ls', len: 7
[2]execve: argv[1] = '/', len: 1
[2]execve: envp[0] = 'TEST_ENV=FROM_INIT', len: 18
[2]execve: envp[1] = 'PWD=/', len: 5
[2]execve: envp[2] = 'TZ=JST-9', len: 8
drwxrwxr-x    1 root wheel   512  6 29 14:31 .
drwxrwxr-x    1 root wheel   512  6 29 14:31 ..
drwxrwxr-x    2 root wheel   896  6 29 14:31 bin
drwxrwxr-x    3 root wheel   384  6 29 14:31 dev
drwxrwxr-x    8 root wheel   128  6 29 14:31 etc
drwxrwxrwx    9 root wheel   128  6 29 14:31 lib
drwxrwxr-x   10 root wheel   192  6 29 14:31 home
drwxrwxr-x   12 root wheel   256  6 29 14:31 usr
# /usr/bin/ls /
[2]execve: argv[0] = '/usr/bin/ls', len: 11
[2]execve: argv[1] = '/', len: 1
[2]execve: envp[0] = 'TEST_ENV=FROM_INIT', len: 18
[2]execve: envp[1] = 'PWD=/', len: 5
[2]execve: envp[2] = 'TZ=JST-9', len: 8
bin  dev  etc  home  lib  usr
# echo $PATH
/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
# ls -l /
[0]execve: argv[0] = 'ls', len: 2
[0]execve: argv[1] = '-l', len: 2
[0]execve: argv[2] = '/', len: 1
[0]execve: envp[0] = 'TEST_ENV=FROM_INIT', len: 18
[0]execve: envp[1] = 'PWD=/', len: 5
[0]execve: envp[2] = 'TZ=JST-9', len: 8
[2]sys_fstatat: flags unimplemented: flags=256	// flags = AT_EMPTY_PATH
ls: cannot access '/': Invalid argument
[1]exit: exit: pid 10, name ls, err 2
CurrentEL: 0x1
DAIF: Debug(1) SError(1) IRQ(1) FIQ(1)
SPSel: 0x1
SPSR_EL1: 0x0
SP: 0xffff00003bffce50
SP_EL0: 0xfffffffffcb0
ELR_EL1: 0x41b510, EC: 0x0, ISS: 0x0.
FAR_EL1: 0x0
Unexpected syscall #130 (unknown)
kern/console.c:283: kernel panic at cpu 0.
```

### sys_fstatat()でflagsが設定している場合、エラーにしていた

- 無視することにした

```
$ /usr/bin/dash
# ls -l /
[2]fileopen: cant namei /etc/passwd
[1]fileopen: cant namei /etc/group
total 4
drwxrwxr-x 1 0 0 896 Jun 29  2022 bin
drwxrwxr-x 1 0 0 384 Jun 29  2022 dev
drwxrwxr-x 1 0 0 128 Jun 29  2022 etc
drwxrwxr-x 1 0 0 192 Jun 29  2022 home
drwxrwxrwx 1 0 0 128 Jun 29  2022 lib
drwxrwxr-x 1 0 0 256 Jun 29  2022 usr
# ls -l /bin
[1]fileopen: cant namei /etc/passwd
[1]fileopen: cant namei /etc/group
total 495
-rwxr-xr-x 1 0 0 39816 Jun 29  2022 bigtest
-rwxr-xr-x 1 0 0 38568 Jun 29  2022 cat
-rwxr-xr-x 1 0 0 48552 Jun 29  2022 date
-rwxr-xr-x 1 0 0 39480 Jun 29  2022 echo
-rwxr-xr-x 1 0 0 44736 Jun 29  2022 init
-rwxr-xr-x 1 0 0 53064 Jun 29  2022 ls
-rwxr-xr-x 1 0 0 51840 Jun 29  2022 mkfs
-rwxr-xr-x 1 0 0 54056 Jun 29  2022 sh
-rwxr-xr-x 1 0 0 45040 Jun 29  2022 sigtest
-rwxr-xr-x 1 0 0 48640 Jun 29  2022 sigtest2
-rwxr-xr-x 1 0 0 22104 Jun 29  2022 sigtest3
-rwxr-xr-x 1 0 0 17744 Jun 29  2022 utest
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

# /binディレクトリのコマンド実行チェック

## ok

```
# /bin/date
2022年 6日21日 火曜日 10時32分17秒 JST
# sigtest3
Got signal!
Hangup
# sigtest2
part1 start
PID 8 function A got 10
PID 8 function A got 999
PID 8 function B got 10

part2 start
PID 8 sends signal to PID 9
PID 9 function C got 10
PID 9 got signal and sends signal to PID 8
PID 8 function C got 10
PID 8 got signal from PID 9

part3 start
PID 10 function D got 10
PID 10 function E got 12
bye bye

all ok
# utest
# echo abc
abc
```

## エラー

```
# echo abc > test
dash: 6: cannot create test: Permission denied
# cat > test
dash: 10: cannot create test: Permission denied
```

### iupdate()にバグ

- ip->modeをdinodeにコピーしていなかった

```
# ls
bin  dev  etc  home  lib  usr
# echo abc > test
# cat test
abc
# ls -l
total 4
drwxrwxr-x 1 root root 896 Jun 30  2022 bin
drwxrwxr-x 1 root root 384 Jun 30  2022 dev
drwxrwxr-x 1 root root 256 Jun 30  2022 etc
drwxrwxr-x 1 root root 192 Jun 30  2022 home
drwxrwxrwx 1 root root 128 Jun 30  2022 lib
-rw-rw-rw- 1 root root   4 Jun 21 10:31 test
drwxrwxr-x 1 root root 256 Jun 30  2022 usr
# rm test
# ls
bin  dev  etc  home  lib  usr
```

```
# bin/sigtest
PID 15 ready
PID 16 ready
PID 17 ready
PID 18 ready
PID 19 ready
PID 19 caught sig 2
unknown error: ecr=0x2000000, il=3355443 (=0x2000000)
PID 18 caught sig 2
kern/console.c:283: kernel panic at cpu 0.

PID 17 caught sig 2
PID 16 caught sig 2
```

### EL1の割り込みでIL=1になり、panicになっていた

- IL=1の場合はNOPとした

```
# sigtest
PID 8 ready
PID 9 ready
PID 10 ready
[3]yield: pid 11 to runable
[3]yield: pid 11 to running
PID 11 ready
PID 12 ready
[1]yield: pid 12 to runable
[1]yield: pid 12 to running
[0]sys_kill: pid=12, sig=2
[0]send_signal: pid=12, sig=2, state=2, paused=1
[0]send_signal: paused! continue
[0]cont_handler: pid=12
[0]wakeup1: pid 12 woke up		// PID=12はCPU=0でwoke up
PID 12 caught sig 2
[2]sys_kill: pid=11, sig=2
[2]send_signal: pid=11, sig=2, state=2, paused=1
[2]send_signal: paused! continue
[2]cont_handler: pid=11
[2]wakeup1: pid 11 woke up		// PID=11はCPU=2でwoke up
PID 11 caught sig 2
[1]sys_kill: pid=10, sig=2
[2]yield: pid 12 to runable
[1]send_signal: pid=10, sig=2, state=2, paused=1
[1]send_signal: paused! continue
[1]cont_handler: pid=10
[1]wakeup1: pid 10 woke up		// PID=10はCPU=1でwoke up
[2]yield: pid 12 to running		<=
PID 10 caught sig 2
[0]sys_kill: pid=9, sig=2
[0]send_signal: pid=9, sig=2, state=2, paused=1
[0]send_signal: paused! continue
[0]cont_handler: pid=9
[0]wakeup1: pid 9 woke up		// PID=9はCPU=0でwoke up
[1]yield: pid 11 to runable
[3]yield: pid 11 to running		<=
PID 9 caught sig 2
[1]sys_kill: pid=8, sig=2
[2]yield: pid 12 to runable
[1]send_signal: pid=8, sig=2, state=2, paused=1
[1]send_signal: paused! continue
[1]cont_handler: pid=8
[1]wakeup1: pid 8 woke up		// PID=8はCPU=1でwoke up
[0]yield: pid 10 to runable
[2]yield: pid 12 to running		<=
PID [83 caug]ht yielsdig:  2pid 11 to runable
[3]yield: pid 10 to running		<=


[1]yield: pid 11 to running		// CPU=1: 11, 10, 12, 8, 11, 9
[0]yield: pid 9 to running		// CPU=0: 9, 11, 9, 10, 8
[2]yield: pid 8 to running		// CPU=2: 8, 9, 10, 12, 10
[3]yield: pid 12 to running		// CPU=3: 12, 8, 11, 9, 12
[1]yield: pid 10 to running
[0]yield: pid 11 to running
[3]yield: pid 8 to running
[1]yield: pid 12 to running
[2]yield: pid 10 to running
[0]yield: pid 9 to running
[3]yield: pid 11 to running
[1]yield: pid 8 to running
[2]yield: pid 12 to running
[0]yield: pid 10 to running
[3]yield: pid 9 to running
[1]yield: pid 11 to running
[0]yield: pid 8 to running
[2]yield: pid 10 to running
[3]yield: pid 12 to running
[1]yield: pid 9 to running
...
```

### 子プロセスがpause()から戻らない
