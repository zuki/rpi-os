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
[1]uvm_map: remap: p=0x430000, *pte =0x3bfbd647                                  // [4] 0x430000がremapなのは当然

[1]sys_mmap: addr=0x0, length=0x4096, prot=0x3, flags=0x22, offset=0x0          // [5]
[1]uvm_map: remap: p=0x431000, *pte =0x3bfc1647

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
[1]uvm_map: remap: p=0x600000000000, *pte =0x3bf2d647						// これが原因でforkに失敗

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

## pipeが動かない件

```
# cat test.txt | head -n 5
[2]sys_pipe2: fd0=3, fd1=4, pipefd[3, 4]
[1]sys_execve: path: /usr/bin/cat
[1]sys_wait4: pid: -1
[2]sys_execve: path: /usr/bin/head
[3]sys_read: fd: 5
[3]sys_write: fd: 1, p: 123
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
, n: 94, f->type: 2
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
[3]sys_read: fd: 5
[3]sys_close: fd: 5
[3]sys_close: fd: 1
[3]sys_close: fd: 2
[3]sys_wait4: pid: -1
[3]sys_read: fd: 0
ls							// プロンプトは出ず
ls							// pipeがcloseされていないっぽい
[3]sys_read: fd: 0
```

## sys_pipe2()のバグだった

- 間違い

```c
int pipefd[2];											// pipefd[2]がカーネル空間に作成され
if (argptr(0, (char **)&pipefd, sizeof(int)*2) < 0)		// そこにfdが設定されのでユーザに返らない
```

- 修正

```c
int *pipefd;											// pipefd[2]はユーザ空間で作成され
if (argptr(0, (char **)&pipefd, sizoef(int)*2) < 0)		// そこにfdを設定するのでユーザに返る
```


```
# cat test.txt | head -n 5
[3]sys_pipe2: pipefd: 0xffff00003bffde78, flags=0x0		// pipefdはカーネル空間アドレス
[3]sys_pipe2: fd0=3, fd1=4, pipefd[3, 4]				// カーネル上は正しいが
pipe: [-296, -1]										// 呼び出し元には正しいfdが帰っていない
```

- argu64で受けるように修正

```
(gdb) n
117	    info("pipe_p: 0x%llx, flags: 0x%x", pipe_p, flags);
(gdb) p/x pipe_p
$1 = 0xfffffffffe78										// ユーザ空間アドレス
```

```
# cat test.txt | head -n 5
[2]sys_pipe2: fd0=3, fd1=4, pipefd[3, 4]
pipe: [3, 4]											// 正しいfdが帰っている
123
111
111
123
999
# cat test.txt | uniq									// これも正しく動く
pipe: [3, 4]
123
111
123
999
あいうえお
999
かきくけこ
12345
あいうえお
# sort test.txt | uniq									// sortはストール
pipe: [3, 4]
# sort test.txt											// sort単体は動く
111
111
123
123
12345
999
999
あいうえお
あいうえお
あいうえお
かきくけこ
```

# gdbが動かなかったのはmacのセキュリティ設定のためだった

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

- kernコンパイル時の最適化オプションを-O0にしたら動いた
- -01, -02はだめ

```
# sigtest
PID 8 ready
PID 9 ready
PID 10 ready
PID 11 ready
PID 12 ready
PID 12 caught sig 2, j 3
PID 11 caught sig 2, j 2
12 is dead
PID 10 caught sig 2, j 1
11 is dead
PID 9 caught sig 2, j 0
10 is dead
PID 8 caught sig 2, j -1
9 is dead
8 is dead
```

## mmaptest

```
# mmaptest
mmap_testスタート
- open mmap.dur (fd=3) READ ONLY
[1] test mmap f
- mmap fd=3 -> 2ページ分PROT_READ MAP_PRIVATEでマップ: addr=0x600000001000
- munmmap 2PAGE: addr=0x600000001000
[1] OK
[2] test mmap private
- mmap fd=3 -> 2ページ分をPAGE PROT_READ/WRITE, MAP_PRIVATEでマップ: addr=0x600000001000
- closed 3
- write 2PAGE = Z
- p[1]=Z
- munmmap 2PAGE: addr=0x600000001000
[2] OK
[3] test mmap read-only
- open mmap.dur (3) RDONALY
[1]sys_mmap: file is not writable
- mmap 3 -> 3PAGE PROT_READ/WRITE MAP_SHARED: addr=0xffffffffffffffff
- close 3
[3] OK
[4] test mmap read/write
- open mmap.dur (3) RDWR
- mmap 3 -> 3PAGE PROT_READ|WRITE MAP_SHARED: addr=0x600000001000
- close 3
- print 2PAGE = Z
- munmmap 2PAGE: addr=0x600000001000
[4] OK
[5] test mmap dirty
- open mmap.dur (3) RDWR
- close 3
[5] OK
[6] test not-mapped unmap
- munmmap PAGE: addr=0x600000003000
[6] OK
[7] test mmap two files
- open mmap1 (3) RDWR/CREAT
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000001000
- close 3
- unlink mmap1
- open mmap2 (3) RDWR/CREAT			// ここでストール
```

### logが満杯か?

-- mmaptest内のprintf()で呼び出されるsys_writev()でストール
-- test 1-6、test 7とforkテストの2つに分けて実行するとどちらも成功

```
# mmaptest
mmap_testスタート
- open mmap.dur (fd=3) READ ONLY
[1] test mmap f
- mmap fd=3 -> 2ページ分PROT_READ MAP_PRIVATEでマップ: addr=0x600000001000
- munmmap 2PAGE: addr=0x600000001000
[1] OK
[2] test mmap private
- mmap fd=3 -> 2ページ分をPAGE PROT_READ/WRITE, MAP_PRIVATEでマップ: addr=0x600000001000
- closed 3
- write 2PAGE = Z
- p[1]=Z
- munmmap 2PAGE: addr=0x600000001000
[2] OK
[3] test mmap read-only
- open mmap.dur (3) RDONALY
[3]sys_mmap: file is not writable
- mmap 3 -> 3PAGE PROT_READ/WRITE MAP_SHARED: addr=0xffffffffffffffff
- close 3
[3] OK
[4] test mmap read/write
- open mmap.dur (3) RDWR
- mmap 3 -> 3PAGE PROT_READ|WRITE MAP_SHARED: addr=0x600000001000
- close 3
- print 2PAGE = Z
- munmmap 2PAGE: addr=0x600000001000
[4] OK
[5] test mmap dirty
- open mmap.dur (3) RDWR
- close 3
[5] OK
[6] test not-mapped unmap
- munmmap PAGE: addr=0x600000003000
[6] OK
mmap_test: Total: 6, OK: 6, NG: 0
mmaptest: all tests succeeded

# mmaptest
mmap_testスタート
[7] test mmap two files
- open mmap1 (3) RDWR/CREAT
- write 3: 12345
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000001000
- close 3
- unlink mmap1
- open mmap2 (3) RDWR/CREAT
- write 3: 67890
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000002000
- close 3
- unlink mmap2
- munmap PAGE: addr=0x600000001000
- munmap PAGE: addr=0x600000002000
[7] OK
mmap_test: Total: 1, OK: 1, NG: 0

fork_test starting
p1[PGSIZE]=A
p2[PGSIZE]=A
fork_test OK
mmaptest: all tests succeeded
```

## mmaptest2

```
# mmaptest2
[F-01] 不正なfdを指定した場合のテスト
 error: Bad file descriptor
 error: Bad file descriptor
 error: Bad file descriptor
[F-01] ok

[F-02] 不正なフラグを指定した場合のテスト
[3]sys_mmap: invalid flags: 0x0
 error: Invalid argument
[F-02] ok

[F-03] readonlyファイルにPROT_WRITEな共有マッピングをした場合のテスト
[0]sys_mmap: file is not writable
 error: Permission denied
[F-03] ok

[F-04] readonlyファイルにPROT_READのみの共有マッピングをした場合のテスト
[F-04] ok

[F-05] PROT指定の異なるプライベートマッピンのテスト
[F-05] ok

[F-06] MMAPTOPを超えるサイズをマッピングした場合のテスト
[2]get_page: get_page readi failed: n=-1, offset=4096, size=4096
copy_page: get_page failed
[2]map_file_page: map_pagecache_page: copy_page failed
 error: Out of memory
[F-06] ok

[F-07] 連続したマッピングを行うテスト			// ここでストール（mmaptestと同じ現象か?)
```

### F-07からテストスタート

```
# mmaptest2

[F-07] 連続したマッピングを行うテスト
[F-07] ok

[F-08] 空のファイルを共有マッピングした場合のテスト
[F-08] ok

[F-09] ファイルが背後にあるプライベートマッピング
[F-09] ok

[F-10] ファイルが背後にある共有マッピングのテスト
[F-10] ok

[F-11] file backed mapping pagecache coherency test
[F-11] ok

[F-12] file backed private mapping with fork test
[F-12] ok

[F-13] file backed shared mapping with fork test
[F-13] failed at strcmp parent: ret2[0]=a, buf[0]=a

[F-14] オフセットを指定したプライベートマッピングのテスト
[F-14] ok

[F-15] file backed valid provided address test
[F-15] ok

[F-16] file backed invalid provided address test
[F-16] failed

[F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合
[F-17] failed: at second mmap

[F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト
ret=0x600000001000
ret2=0x600000003000
not mapped
kern/console.c:283: kernel panic at cpu 2.

[F-19] ２つのマッピングの間に不可能なアドレスを指定した場合
not mapped
kern/console.c:283: kernel panic at cpu 1.
```

### anonymous_test()

```
# mmaptest2

[A-01] anonymous private mapping test
p[0]=0, p[2499]=2499
[A-01] ok

[A-02] anonymous shared mapping test						// 結果が出ていない

[A-03] anonymous private mapping with fork test
[A-03] ok

[A-04] anonymous shared mapping with multiple forks test
[3]exit: exit: pid 12, name , err 1
[0]exit: exit: pid 11, name , err 1
[1]exit: exit: pid 10, name , err 1
[A-04] failed at strcmp fork 1 parent

[A-05] anonymous private & shared mapping together with fork test
[0]exit: exit: pid 13, name , err 1
[A-05] failed at strcmp share

[A-06] anonymous missing flags test
[3]sys_mmap: invalid flags: 0x20
[A-06] ok

[A-07] anonymous exceed mapping count test
[A-07] ok

[A-08] anonymous exceed mapping size test			// ここでストール.[A-08]は単独でもストール
```

### [A-09]から実行

```
[A-09] anonymous zero size mapping test
[A-09] failed

[A-10] anonymous valid provided address test
[A-10] ok

[A-11] anonymous invalid provided address test
[A-11] failed at mmap

[A-12] anonymous overlapping provided address test
[A-12] failed at second mmap

[A-13] anonymous intermediate provided address test
not mapped
kern/console.c:283: kernel panic at cpu 2.
```

### other_test

```
# mmaptest2

[O-01] munmap only partial size test
[O-01] ok

[O-02] write on read only mapping test
[O-02] ok

[O-03] none permission on mapping test
[O-03] ok

[O-04] mmap valid address map fixed flag test
[O-04] ok

[O-05] mmap invalid address map fixed flag test
[O-05] failed at mmap 1
```

###  7/4

- MAP_FIXEDで指定したアドレスが存在する場合はエラーに変更
- MAP_SHAREDの場合は、free_mmap_list()しない
- sys_msyncを実装
- MAXOPBLOCKSを42に変更
- mmaptestはF-19がregression

```
# mmaptest2
[F-01] 不正なfdを指定した場合のテスト
 error: Bad file descriptor
 error: Bad file descriptor
 error: Bad file descriptor
[F-01] ok

[F-02] 不正なフラグを指定した場合のテスト
[0]sys_mmap: invalid flags: 0x0
 error: Invalid argument
[F-02] ok

[F-03] readonlyファイルにPROT_WRITEな共有マッピングをした場合のテスト
[3]sys_mmap: file is not writable
 error: Permission denied
[F-03] ok

[F-04] readonlyファイルにPROT_READのみの共有マッピングをした場合のテスト
[F-04] ok

[F-05] PROT指定の異なるプライベートマッピンのテスト
[F-05] ok

[F-06] MMAPTOPを超えるサイズをマッピングした場合のテスト
[1]get_page: get_page readi failed: n=-1, offset=4096, size=4096
[1]copy_page: get_page failed
[1]map_file_page: map_pagecache_page: copy_page failed
 error: Out of memory
[F-06] ok

[F-07] 連続したマッピングを行うテスト
[0]uvm_map: remap: p=0x600000001000, *pte =0x3bb3c647

 error: Invalid argument
[F-07] failed at 1 total mappings

[F-08] 空のファイルを共有マッピングした場合のテスト
[F-08] ok

[F-09] ファイルが背後にあるプライベートマッピング
```

```
[F-09] ファイルが背後にあるプライベートマッピング
[F-09] ok

[F-10] ファイルが背後にある共有マッピングのテスト
[F-10] ok

[F-11] file backed mapping pagecache coherency test
[F-11] ok

[F-12] file backed private mapping with fork test
[F-12] ok

[F-13] file backed shared mapping with fork test
[F-13] failed at strcmp parent: ret2[0]=a, buf[0]=a

[F-14] オフセットを指定したプライベートマッピングのテスト
[F-14] ok

[F-15] file backed valid provided address test
[F-15] ok

[F-16] file backed invalid provided address test
[F-16] ok

[F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合
[F-17] ok

[F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト
[F-18] ok

[F-19] ２つのマッピングの間に不可能なアドレスを指定した場合
ret =0x600000001000
ret2=0x600000003000
[2]get_page: get_page readi failed: n=-1, offset=8192, size=4096
[2]copy_page: get_page failed
[2]map_file_page: map_pagecache_page: copy_page failed
ret3=0xffffffffffffffff
[F-19] failed at third mmap

[F-20] 共有マッピングでファイル容量より大きなサイズを指定した場合
[F-20] ok

[F-21] write onlyファイルへのREAD/WRITE共有マッピングのテスト
[0]sys_mmap: file is not readable
[F-21] ok
```

```
# mmaptest2

[A-01] anonymous private mapping test
p[0]=0, p[2499]=2499
[A-01] ok

[A-02] anonymous shared mapping test						// MAP_ANON + MAP_SHARE + fork()が絡むとエラー
[A-02] p1[1]: 0 != 1

[A-03] anonymous private mapping with fork test
[A-03] ok

[A-04] anonymous shared mapping with multiple forks test
[2]exit: exit: pid 12, name , err 1
[3]exit: exit: pid 11, name , err 1
[3]exit: exit: pid 10, name , err 1
[A-04] failed at strcmp fork 1 parent

[A-05] anonymous private & shared mapping together with fork test
[1]exit: exit: pid 13, name , err 1
[A-05] failed at strcmp share

[A-06] anonymous missing flags test
[0]sys_mmap: invalid flags: 0x20
[A-06] ok

[A-07] anonymous exceed mapping count test
[A-07] ok

[A-08] anonymous exceed mapping size test
[A-08] ok

[A-09] anonymous zero size mapping test
[0]sys_mmap: invalid length: 0 or offset: 0
[A-09] ok

[A-10] anonymous valid provided address test
[A-10] ok

[A-11] anonymous invalid provided address test
[A-11] ok: not running because of an invalid test

[A-12] anonymous overlapping provided address test
[A-12] ok

[A-13] anonymous intermediate provided address test
[A-13] ok

[A-14] anonymous intermediate provided address not possible test
[A-14] ok

[O-01] munmap only partial size test
[O-01] ok

[O-02] write on read only mapping test
[O-02] ok

[O-03] none permission on mapping test
[O-03] ok

[O-04] mmap valid address map fixed flag test
[O-04] ok

[O-05] mmap invalid address map fixed flag test
[0]mmap: fixed address should be page align: 0x600000000100
[0]mmap: addr is used
[O-05] test ok

file_test:  ok: 0, ng: 0
anon_test:  ok: 10, ng: 3
other_test: ok: 5, ng: 0
```

### fudanではOK

```
[F-13] file backed shared mapping with fork test
child : ret2[0, 49, 50]=[o, o, a], buf=[a, a, a]
parent: ret2[0, 49, 50]=[o, o, a], buf=[a, a, a]
[F-13] ok
```

### copy_mmap_listでMAP_SHAREDの場合にmap_file_pagesしようとした

- ret2の親から子へのデータコピーは終わっている
- map_file_pagesは無用

```
(gdb) x/16xg 0xffff00003bbb6000 	// ret2のアドレス
0xffff00003bbb6000:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6010:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6020:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6030:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6040:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6050:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6060:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6070:	0x6161616161616161	0x6161616161616161
(gdb)
0xffff00003bbb6080:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6090:	0x6161616161616161	0x6161616161616161
0xffff00003bbb60a0:	0x6161616161616161	0x6161616161616161
0xffff00003bbb60b0:	0x6161616161616161	0x6161616161616161
0xffff00003bbb60c0:	0x6161616161616161	0x0000000000000000
0xffff00003bbb60d0:	0x0000000000000000	0x0000000000000000
0xffff00003bbb60e0:	0x0000000000000000	0x0000000000000000
0xffff00003bbb60f0:	0x0000000000000000	0x0000000000000000
(gdb) x/16xg page->page + dest_offset	// page cacheのアドレス
0xffff00003bfff000:	0x6161616161616161	0x6161616161616161
0xffff00003bfff010:	0x6161616161616161	0x6161616161616161
0xffff00003bfff020:	0x6161616161616161	0x6161616161616161
0xffff00003bfff030:	0x6161616161616161	0x6161616161616161
0xffff00003bfff040:	0x6161616161616161	0x6161616161616161
0xffff00003bfff050:	0x6161616161616161	0x6161616161616161
0xffff00003bfff060:	0x6161616161616161	0x6161616161616161
0xffff00003bfff070:	0x6161616161616161	0x6161616161616161
(gdb)
0xffff00003bfff080:	0x6161616161616161	0x6161616161616161
0xffff00003bfff090:	0x6161616161616161	0x6161616161616161
0xffff00003bfff0a0:	0x6161616161616161	0x6161616161616161
0xffff00003bfff0b0:	0x6161616161616161	0x6161616161616161
0xffff00003bfff0c0:	0x6161616161616161	0x6161616161616161
0xffff00003bfff0d0:	0x6161616161616161	0x6161616161616161
0xffff00003bfff0e0:	0x6161616161616161	0x6161616161616161
0xffff00003bfff0f0:	0x6161616161616161	0x6161616161616161
```

### MAP_SHAREDの場合

- copy_mmap_list()でpteのpaを付け替え

```c
if (node->flags & MAP_SHARED) {
	ptep = pgdir_walk(parent->pgdir, node->addr, 0);
	if (!ptep) panic("parent pgdir not pte: va=0x%p\n", region->addr);
	ptec = pgdir_walk(child->pgdir, region->addr, 0);
	if (!ptec) panic("child  pgdir not pte: va=0x%p\n", region->addr);
	uint64_t pac = PTE_ADDR(*ptec);
	kfree(P2V(pac));
	*ptec = *ptep;
}
```

```
[F-13] file backed shared mapping with fork test
child : ret2[0, 1, 49, 50]=[o, o, o, a], buf=[a, a, a, a]

00f0b33b0000ffff6f6f6f6f6f6f6f6f
6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f
6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f
6f6f61
[F-13] failed at strcmp 3: buf2[0, 49, 50]=[, o, a], buf[0, 49, 50]=[o, o, a]
```

- debug

```
(gdb) c
Continuing.
[Switching to Thread 1.3]

Thread 3 hit Breakpoint 1, copy_mmap_list (parent=0xffff0000000e3318 <ptable+9408>, child=0xffff0000000e3938 <ptable+10976>) at kern/mmap.c:664
664	            ptep = pgdir_walk(parent->pgdir, node->addr, 0);
(gdb) b exit
Breakpoint 2 at 0xffff000000095e60: file kern/proc.c, line 439.
(gdb) b wait4
Breakpoint 3 at 0xffff000000095c38: file kern/proc.c, line 388.
(gdb) x/16gx 0xffff00003bb3f000
0xffff00003bb3f000:	0x000000003bb40003	0x0000000000000000
0xffff00003bb3f010:	0x0000000000000000	0x0000000000000000
0xffff00003bb3f020:	0x0000000000000000	0x0000000000000000
0xffff00003bb3f030:	0x0000000000000000	0x0000000000000000
0xffff00003bb3f040:	0x0000000000000000	0x0000000000000000
0xffff00003bb3f050:	0x0000000000000000	0x0000000000000000
0xffff00003bb3f060:	0x0000000000000000	0x0000000000000000
0xffff00003bb3f070:	0x0000000000000000	0x0000000000000000
(gdb) n
665	            if (!ptep) panic("parent pgdir not pte: va=0x%p\n", region->addr);
(gdb)
666	            ptec = pgdir_walk(child->pgdir, region->addr, 0);
(gdb)
667	            if (!ptec) panic("child  pgdir not pte: va=0x%p\n", region->addr);
(gdb)
668	            uint64_t pac = PTE_ADDR(*ptec);
(gdb) p/x ptep
$1 = 0xffff00003bb45008
(gdb) p/x *ptep
$2 = 0x3bbb6647
(gdb) p/x ptec
$3 = 0xffff00003bb58008
(gdb) p/x *ptec
$4 = 0x3bb55647
(gdb) x/16gx ptep
0xffff00003bb45008:	0x000000003bbb6647	0x0000000000000000
0xffff00003bb45018:	0x0000000000000000	0x0000000000000000
0xffff00003bb45028:	0x0000000000000000	0x0000000000000000
0xffff00003bb45038:	0x0000000000000000	0x0000000000000000
0xffff00003bb45048:	0x0000000000000000	0x0000000000000000
0xffff00003bb45058:	0x0000000000000000	0x0000000000000000
0xffff00003bb45068:	0x0000000000000000	0x0000000000000000
0xffff00003bb45078:	0x0000000000000000	0x0000000000000000
(gdb) x/16gx ptec
0xffff00003bb58008:	0x000000003bb55647	0x0000000000000000
0xffff00003bb58018:	0x0000000000000000	0x0000000000000000
0xffff00003bb58028:	0x0000000000000000	0x0000000000000000
0xffff00003bb58038:	0x0000000000000000	0x0000000000000000
0xffff00003bb58048:	0x0000000000000000	0x0000000000000000
0xffff00003bb58058:	0x0000000000000000	0x0000000000000000
0xffff00003bb58068:	0x0000000000000000	0x0000000000000000
0xffff00003bb58078:	0x0000000000000000	0x0000000000000000
(gdb) x/16gx 0xffff00003bbb6000
0xffff00003bbb6000:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6010:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6020:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6030:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6040:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6050:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6060:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6070:	0x6161616161616161	0x6161616161616161
(gdb) x/16gx 0xffff00003bb55000
0xffff00003bb55000:	0x6161616161616161	0x6161616161616161
0xffff00003bb55010:	0x6161616161616161	0x6161616161616161
0xffff00003bb55020:	0x6161616161616161	0x6161616161616161
0xffff00003bb55030:	0x6161616161616161	0x6161616161616161
0xffff00003bb55040:	0x6161616161616161	0x6161616161616161
0xffff00003bb55050:	0x6161616161616161	0x6161616161616161
0xffff00003bb55060:	0x6161616161616161	0x6161616161616161
0xffff00003bb55070:	0x6161616161616161	0x6161616161616161
(gdb) n
669	            kfree(P2V(pac));
(gdb) p/x pac
$5 = 0x3bb55000
(gdb) n
670	            *ptec = PTE_ADDR(*ptep) | get_perm(region->prot, region->flags);
(gdb) x/16gx 0xffff00003bb55000
0xffff00003bb55000:	0xffff00003bb66000	0x6161616161616161
0xffff00003bb55010:	0x6161616161616161	0x6161616161616161
0xffff00003bb55020:	0x6161616161616161	0x6161616161616161
0xffff00003bb55030:	0x6161616161616161	0x6161616161616161
0xffff00003bb55040:	0x6161616161616161	0x6161616161616161
0xffff00003bb55050:	0x6161616161616161	0x6161616161616161
0xffff00003bb55060:	0x6161616161616161	0x6161616161616161
0xffff00003bb55070:	0x6161616161616161	0x6161616161616161
(gdb) n
672	        if (cnode == 0)
(gdb) p/x ptec
$6 = 0xffff00003bb58008
(gdb) p/x *ptec
$7 = 0x6000003bbb6747
(gdb) x/16gx 0xffff00003bbb6000
0xffff00003bbb6000:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6010:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6020:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6030:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6040:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6050:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6060:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6070:	0x6161616161616161	0x6161616161616161
(gdb) n
675	            tail->next = region;
(gdb) c
Continuing.

Thread 3 hit Breakpoint 3, wait4 (pid=-1, status=0xfffffffffdac, options=0, ru=0x0) at kern/proc.c:388
388	    struct proc *cp = thisproc();
(gdb) c
Continuing.
[Switching to Thread 1.1]

Thread 1 hit Breakpoint 2, exit (err=0) at kern/proc.c:439
439	    struct proc *cp = thisproc();
(gdb) c
Continuing.
[Switching to Thread 1.2]

Thread 2 hit Breakpoint 2, exit (err=0) at kern/proc.c:439
439	    struct proc *cp = thisproc();
(gdb) c
Continuing.
[Switching to Thread 1.1]

Thread 1 hit Breakpoint 3, wait4 (pid=-1, status=0xfffffffffc6c, options=3, ru=0x0) at kern/proc.c:388
388	    struct proc *cp = thisproc();
(gdb) x/16gx 0xffff00003bbb6000
0xffff00003bbb6000:	0xffff00003bb3d000	0x6f6f6f6f6f6f6f6f
0xffff00003bbb6010:	0x6f6f6f6f6f6f6f6f	0x6f6f6f6f6f6f6f6f
0xffff00003bbb6020:	0x6f6f6f6f6f6f6f6f	0x6f6f6f6f6f6f6f6f
0xffff00003bbb6030:	0x6161616161616f6f	0x6161616161616161
0xffff00003bbb6040:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6050:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6060:	0x6161616161616161	0x6161616161616161
0xffff00003bbb6070:	0x6161616161616161	0x6161616161616161
(gdb) c
Continuing.
[Inferior 1 (process 1) exited normally]
(gdb) q
```

- pgdir

```
# mmaptest2

[3]uvm_copy: pgdir[0x0] pgt1=0xffff00003bbaf000								// dash -> mmaptest2のforkのuvm_copy
[3]uvm_copy: pgdir[0x0] pgt2=0xffff00003bbae000
[3]uvm_copy: pgdir[0x400000] pgt3=0xffff00003bbad000
[3]uvm_copy: pgdir[0x400000] pte=0xffff00003bbb0000, np=0xffff00003bbb7000
[3]uvm_copy: pgdir[0x401000] pte=0xffff00003bbac000, np=0xffff00003bbbe000
[3]uvm_copy: pgdir[0x402000] pte=0xffff00003bbab000, np=0xffff00003bbfa000
[3]uvm_copy: pgdir[0x403000] pte=0xffff00003bbaa000, np=0xffff00003bbfe000
[3]uvm_copy: pgdir[0x404000] pte=0xffff00003bba9000, np=0xffff00003bbc7000
[3]uvm_copy: pgdir[0x405000] pte=0xffff00003bba8000, np=0xffff00003bbbf000
[3]uvm_copy: pgdir[0x406000] pte=0xffff00003bba7000, np=0xffff00003bbc0000
[3]uvm_copy: pgdir[0x407000] pte=0xffff00003bba6000, np=0xffff00003bbc1000
[3]uvm_copy: pgdir[0x408000] pte=0xffff00003bba5000, np=0xffff00003bbc2000
[3]uvm_copy: pgdir[0x409000] pte=0xffff00003bba4000, np=0xffff00003bbc3000
[3]uvm_copy: pgdir[0x40a000] pte=0xffff00003bba3000, np=0xffff00003bbc4000
[3]uvm_copy: pgdir[0x40b000] pte=0xffff00003bba2000, np=0xffff00003bbc5000
[3]uvm_copy: pgdir[0x40c000] pte=0xffff00003bba1000, np=0xffff00003bbc6000
[3]uvm_copy: pgdir[0x40d000] pte=0xffff00003bba0000, np=0xffff00003bbfb000
[3]uvm_copy: pgdir[0x40e000] pte=0xffff00003bb9f000, np=0xffff00003bb70000
[3]uvm_copy: pgdir[0x40f000] pte=0xffff00003bb9e000, np=0xffff00003bb6f000
[3]uvm_copy: pgdir[0x410000] pte=0xffff00003bb9d000, np=0xffff00003bb6e000
[3]uvm_copy: pgdir[0x411000] pte=0xffff00003bb9c000, np=0xffff00003bb6d000
[3]uvm_copy: pgdir[0x412000] pte=0xffff00003bb9b000, np=0xffff00003bb6c000
[3]uvm_copy: pgdir[0x413000] pte=0xffff00003bb9a000, np=0xffff00003bb6b000
[3]uvm_copy: pgdir[0x414000] pte=0xffff00003bb99000, np=0xffff00003bb6a000
[3]uvm_copy: pgdir[0x415000] pte=0xffff00003bb98000, np=0xffff00003bb69000
[3]uvm_copy: pgdir[0x416000] pte=0xffff00003bb97000, np=0xffff00003bb68000
[3]uvm_copy: pgdir[0x417000] pte=0xffff00003bb96000, np=0xffff00003bb67000
[3]uvm_copy: pgdir[0x418000] pte=0xffff00003bb95000, np=0xffff00003bb66000
[3]uvm_copy: pgdir[0x419000] pte=0xffff00003bb94000, np=0xffff00003bb65000
[3]uvm_copy: pgdir[0x41a000] pte=0xffff00003bb93000, np=0xffff00003bb64000
[3]uvm_copy: pgdir[0x41b000] pte=0xffff00003bb92000, np=0xffff00003bb63000
[3]uvm_copy: pgdir[0x41c000] pte=0xffff00003bb91000, np=0xffff00003bb62000
[3]uvm_copy: pgdir[0x41d000] pte=0xffff00003bb90000, np=0xffff00003bb61000
[3]uvm_copy: pgdir[0x41e000] pte=0xffff00003bb8f000, np=0xffff00003bb60000
[3]uvm_copy: pgdir[0x41f000] pte=0xffff00003bb8e000, np=0xffff00003bb5f000
[3]uvm_copy: pgdir[0x420000] pte=0xffff00003bb8d000, np=0xffff00003bb5e000
[3]uvm_copy: pgdir[0x421000] pte=0xffff00003bb8c000, np=0xffff00003bb5d000
[3]uvm_copy: pgdir[0x422000] pte=0xffff00003bb8b000, np=0xffff00003bb5c000
[3]uvm_copy: pgdir[0x423000] pte=0xffff00003bb8a000, np=0xffff00003bb5b000
[3]uvm_copy: pgdir[0x424000] pte=0xffff00003bb89000, np=0xffff00003bb5a000
[3]uvm_copy: pgdir[0x425000] pte=0xffff00003bb88000, np=0xffff00003bb59000
[3]uvm_copy: pgdir[0x426000] pte=0xffff00003bb87000, np=0xffff00003bb58000
[3]uvm_copy: pgdir[0x427000] pte=0xffff00003bb86000, np=0xffff00003bb57000
[3]uvm_copy: pgdir[0x428000] pte=0xffff00003bb85000, np=0xffff00003bb56000
[3]uvm_copy: pgdir[0x429000] pte=0xffff00003bb84000, np=0xffff00003bb55000
[3]uvm_copy: pgdir[0x42a000] pte=0xffff00003bb83000, np=0xffff00003bb54000
[3]uvm_copy: pgdir[0x42b000] pte=0xffff00003bb82000, np=0xffff00003bb53000
[3]uvm_copy: pgdir[0x42c000] pte=0xffff00003bb81000, np=0xffff00003bb52000
[3]uvm_copy: pgdir[0x42d000] pte=0xffff00003bb80000, np=0xffff00003bb51000
[3]uvm_copy: pgdir[0x42e000] pte=0xffff00003bb7f000, np=0xffff00003bb50000
[3]uvm_copy: pgdir[0x42f000] pte=0xffff00003bb7e000, np=0xffff00003bb4f000
[3]uvm_copy: pgdir[0x430000] pte=0xffff00003bbfc000, np=0xffff00003bb4e000
[3]uvm_copy: pgdir[0x431000] pte=0xffff00003bbbd000, np=0xffff00003bb4d000
[3]uvm_copy: pgdir[0x600000000000] pgt1=0xffff00003bbb2000
[3]uvm_copy: pgdir[0x600000000000] pgt2=0xffff00003bbb3000
[3]uvm_copy: pgdir[0x600000000000] pgt3=0xffff00003bbb4000
[3]uvm_copy: pgdir[0x600000000000] pte=0xffff00003bbbb000, np=0xffff00003bb4c000
[3]uvm_copy: pgdir[0xff8000000000] pgt1=0xffff00003bb7d000
[3]uvm_copy: pgdir[0xffffc0000000] pgt2=0xffff00003bb7c000
[3]uvm_copy: pgdir[0xffffffe00000] pgt3=0xffff00003bb7b000
[3]uvm_copy: pgdir[0xffffffff6000] pte=0xffff00003bb79000, np=0xffff00003bb48000
[3]uvm_copy: pgdir[0xffffffff7000] pte=0xffff00003bb78000, np=0xffff00003bb44000
[3]uvm_copy: pgdir[0xffffffff8000] pte=0xffff00003bb77000, np=0xffff00003bb43000
[3]uvm_copy: pgdir[0xffffffff9000] pte=0xffff00003bb76000, np=0xffff00003bb42000
[3]uvm_copy: pgdir[0xffffffffa000] pte=0xffff00003bb75000, np=0xffff00003bb41000
[3]uvm_copy: pgdir[0xffffffffb000] pte=0xffff00003bb74000, np=0xffff00003bb40000
[3]uvm_copy: pgdir[0xffffffffc000] pte=0xffff00003bb73000, np=0xffff00003bb3f000	// このnpのアドレスがret2の先頭に設定されている
[3]uvm_copy: pgdir[0xffffffffd000] pte=0xffff00003bb72000, np=0xffff00003bb3e000
[3]uvm_copy: pgdir[0xffffffffe000] pte=0xffff00003bb71000, np=0xffff00003bb3d000
[3]uvm_copy: pgdir[0xfffffffff000] pte=0xffff00003bb7a000, np=0xffff00003bb3c000

[F-13] file backed shared mapping with fork test
																			// F-13内のforkのuvm_copy
[0]uvm_copy: pgdir[0x0] pgt1=0xffff00003bb39000								// 1: ここはheap領域(sys_brkで拡張した部分)
[0]uvm_copy: pgdir[0x0] pgt2=0xffff00003bb38000
[0]uvm_copy: pgdir[0x400000] pgt3=0xffff00003bb37000
[0]uvm_copy: pgdir[0x400000] pte=0xffff00003bb3a000, np=0xffff00003bb3e000
[0]uvm_copy: pgdir[0x401000] pte=0xffff00003bb36000, np=0xffff00003bb42000
[0]uvm_copy: pgdir[0x402000] pte=0xffff00003bb35000, np=0xffff00003bb43000
[0]uvm_copy: pgdir[0x403000] pte=0xffff00003bb34000, np=0xffff00003bb44000
[0]uvm_copy: pgdir[0x404000] pte=0xffff00003bb33000, np=0xffff00003bb48000
[0]uvm_copy: pgdir[0x405000] pte=0xffff00003bb32000, np=0xffff00003bb4b000
[0]uvm_copy: pgdir[0x406000] pte=0xffff00003bb31000, np=0xffff00003bb4a000
[0]uvm_copy: pgdir[0x407000] pte=0xffff00003bb30000, np=0xffff00003bb49000
[0]uvm_copy: pgdir[0x408000] pte=0xffff00003bb2f000, np=0xffff00003bb4c000
[0]uvm_copy: pgdir[0x409000] pte=0xffff00003bb2e000, np=0xffff00003bbb8000
[0]uvm_copy: pgdir[0x40a000] pte=0xffff00003bb2d000, np=0xffff00003bbb9000
[0]uvm_copy: pgdir[0x40b000] pte=0xffff00003bb2c000, np=0xffff00003bbba000
[0]uvm_copy: pgdir[0x40c000] pte=0xffff00003bb2b000, np=0xffff00003bb4d000
[0]uvm_copy: pgdir[0x40d000] pte=0xffff00003bb2a000, np=0xffff00003bb4e000
[0]uvm_copy: pgdir[0x40e000] pte=0xffff00003bb29000, np=0xffff00003bb4f000
[0]uvm_copy: pgdir[0x40f000] pte=0xffff00003bb28000, np=0xffff00003bb50000
[0]uvm_copy: pgdir[0x410000] pte=0xffff00003bb27000, np=0xffff00003bb51000
[0]uvm_copy: pgdir[0x411000] pte=0xffff00003bb26000, np=0xffff00003bb52000
[0]uvm_copy: pgdir[0x412000] pte=0xffff00003bb25000, np=0xffff00003bb53000
[0]uvm_copy: pgdir[0x413000] pte=0xffff00003bb24000, np=0xffff00003bb54000

[0]uvm_copy: pgdir[0x600000000000] pgt1=0xffff00003bb47000					// 2: ここがret2 = mmap()の部分
[0]uvm_copy: pgdir[0x600000000000] pgt2=0xffff00003bb46000
[0]uvm_copy: pgdir[0x600000000000] pgt3=0xffff00003bb45000
[0]uvm_copy: pgdir[0x600000001000] pte=0xffff00003bbb6000, np=0xffff00003bb55000	// pteは4, npは5

[0]uvm_copy: pgdir[0xff8000000000] pgt1=0xffff00003bb23000					// 3: ここはスタック部分
[0]uvm_copy: pgdir[0xffffc0000000] pgt2=0xffff00003bb22000
[0]uvm_copy: pgdir[0xffffffe00000] pgt3=0xffff00003bb21000
[0]uvm_copy: pgdir[0xffffffff6000] pte=0xffff00003bb1f000, np=0xffff00003bb59000
[0]uvm_copy: pgdir[0xffffffff7000] pte=0xffff00003bb1e000, np=0xffff00003bb5d000
[0]uvm_copy: pgdir[0xffffffff8000] pte=0xffff00003bb1d000, np=0xffff00003bb5e000
[0]uvm_copy: pgdir[0xffffffff9000] pte=0xffff00003bb1c000, np=0xffff00003bb5f000
[0]uvm_copy: pgdir[0xffffffffa000] pte=0xffff00003bb1b000, np=0xffff00003bb60000
[0]uvm_copy: pgdir[0xffffffffb000] pte=0xffff00003bb1a000, np=0xffff00003bb61000
[0]uvm_copy: pgdir[0xffffffffc000] pte=0xffff00003bb19000, np=0xffff00003bb62000
[0]uvm_copy: pgdir[0xffffffffd000] pte=0xffff00003bb18000, np=0xffff00003bb63000
[0]uvm_copy: pgdir[0xffffffffe000] pte=0xffff00003bb17000, np=0xffff00003bb64000
[0]uvm_copy: pgdir[0xfffffffff000] pte=0xffff00003bb20000, np=0xffff00003bb65000

[0]copy_mmap_list: ptep=0xffff00003bb45008, *ptep=0x3bbb6647				// 4: 親のpteと*pte
[0]copy_mmap_list: ptec=0xffff00003bb58008, *ptec=0x3bb55647				// 5: 子のpteと*pte
child : ret2[0, 1, 49, 50]=[o, o, o, a], buf=[a, a, a, a]

00f0b33b0000ffff6f6f6f6f6f6f6f6f
6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f
6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f
6f6f61
[F-13] failed at strcmp 3: buf2[0, 49, 50]=[, o, a], buf[0, 49, 50]=[o, o, a]
```

- uvm_copyでpteをuvm_mapする際のpgtの作成部分も出力

```
[F-13] file backed shared mapping with fork test

[0]uvm_copy: pgdir[0x600000000000] pgt1=0xffff00003bb47000
[0]uvm_copy: pgdir[0x600000000000] pgt2=0xffff00003bb46000
[0]uvm_copy: pgdir[0x600000000000] pgt3=0xffff00003bb45000
[0]uvm_copy: pgdir[0x600000001000] pte=0xffff00003bbb6000, np=0xffff00003bb55000
[0]pgdir_walk: alloc: pgt0=0xffff00003bb56000
[0]pgdir_walk: alloc: pgt1=0xffff00003bb57000
[0]pgdir_walk: alloc: pgt2=0xffff00003bb58000

[0]copy_mmap_list: ptep=0xffff00003bb45008, *ptep=0x3bbb6647
[0]copy_mmap_list: ptec=0xffff00003bb58008, *ptec=0x3bb55647

child : ret2[0, 1, 49, 50]=[o, o, o, a], buf=[a, a, a, a]

00f0b33b0000ffff6f6f6f6f6f6f6f6f
6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f
6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f
6f6f61
[F-13] failed at strcmp 3: buf2[0, 49, 50]=[, o, a], buf[0, 49, 50]=[o, o, a]
```

- F-13のディスアセンブル

```
0000000000401cd4 <file_shared_with_fork_test>:
  401cd4:	a9a37bfd 	stp	x29, x30, [sp, #-464]!
  401cd8:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401cdc:	911c6000 	add	x0, x0, #0x718
  401ce0:	910003fd 	mov	x29, sp
  401ce4:	f90013f5 	str	x21, [sp, #32]
  401ce8:	b0000075 	adrp	x21, 40e000 <__stdio_ofl_lockptr+0xa68>
  401cec:	94000bd8 	bl	404c4c <puts>
  401cf0:	f942eea0 	ldr	x0, [x21, #1496]
  401cf4:	52800041 	mov	w1, #0x2                   	// #2
  401cf8:	94000a78 	bl	4046d8 <open>
  401cfc:	3100041f 	cmn	w0, #0x1
  401d00:	54000680 	b.eq	401dd0 <file_shared_with_fork_test+0xfc>  // b.none
  401d04:	910103e1 	add	x1, sp, #0x40
  401d08:	d2801902 	mov	x2, #0xc8                  	// #200
  401d0c:	a90153f3 	stp	x19, x20, [sp, #16]
  401d10:	2a0003f3 	mov	w19, w0			# w19 = fd
  401d14:	94001416 	bl	406d6c <read>
  401d18:	aa0003e1 	mov	x1, x0			# x1 = size
  401d1c:	f103201f 	cmp	x0, #0xc8
  401d20:	54000401 	b.ne	401da0 <file_shared_with_fork_test+0xcc>  // b.any
  401d24:	2a1303e4 	mov	w4, w19						// fd
  401d28:	d2800005 	mov	x5, #0x0                   	// #0
  401d2c:	52800023 	mov	w3, #0x1                   	// #1 MAP_SHARED
  401d30:	52800062 	mov	w2, #0x3                   	// #3 PROT_READ/WRITE
  401d34:	d2800000 	mov	x0, #0x0                   	// #0
  401d38:	94000a9f 	bl	4047b4 <__mmap>
  401d3c:	aa0003f4 	mov	x20, x0			# x20 = ret2
  401d40:	94000aea 	bl	4048e8 <fork>
  401d44:	350005c0 	cbnz	w0, 401dfc <file_shared_with_fork_test+0x128>
											# 子プロセス
  401d48:	4f03e5e0 	movi	v0.16b, #0x6f	# v0 = 'o' x 16
  401d4c:	d1000683 	sub	x3, x20, #0x1		# x3 = ret2 - 1
  401d50:	d2800021 	mov	x1, #0x1                   	// #1
  401d54:	7d006280 	str	h0, [x20, #48]		# ret2[48, 49] = ['o', 'o']
  401d58:	ad000280 	stp	q0, q0, [x20]		# ret2[0:31] = '0'
  401d5c:	3d800a80 	str	q0, [x20, #32]		# ret2[32:47] = '0'

  401d60:	910103e0 	add	x0, sp, #0x40
  401d64:	8b010000 	add	x0, x0, x1
  401d68:	38616862 	ldrb	w2, [x3, x1]
  401d6c:	385ff000 	ldurb	w0, [x0, #-1]
  401d70:	6b00005f 	cmp	w2, w0
  401d74:	54000121 	b.ne	401d98 <file_shared_with_fork_test+0xc4>  // b.any
  401d78:	91000421 	add	x1, x1, #0x1
  401d7c:	f103243f 	cmp	x1, #0xc9
  401d80:	54ffff01 	b.ne	401d60 <file_shared_with_fork_test+0x8c>  // b.any
  401d84:	394103e2 	ldrb	w2, [sp, #64]
  401d88:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401d8c:	52800de1 	mov	w1, #0x6f                  	// #111
  401d90:	91206000 	add	x0, x0, #0x818
  401d94:	94000b8e 	bl	404bcc <printf>
  401d98:	52800000 	mov	w0, #0x0                   	// #0
  401d9c:	97fff89d 	bl	400010 <exit>
  401da0:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401da4:	911dc000 	add	x0, x0, #0x770
  401da8:	94000ba9 	bl	404c4c <puts>
  401dac:	b0000060 	adrp	x0, 40e000 <__stdio_ofl_lockptr+0xa68>
  401db0:	911f4000 	add	x0, x0, #0x7d0
  401db4:	a94153f3 	ldp	x19, x20, [sp, #16]
  401db8:	b9400401 	ldr	w1, [x0, #4]
  401dbc:	11000421 	add	w1, w1, #0x1
  401dc0:	b9000401 	str	w1, [x0, #4]
  401dc4:	f94013f5 	ldr	x21, [sp, #32]
  401dc8:	a8dd7bfd 	ldp	x29, x30, [sp], #464
  401dcc:	d65f03c0 	ret
  401dd0:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401dd4:	911d4000 	add	x0, x0, #0x750
  401dd8:	94000b9d 	bl	404c4c <puts>
  401ddc:	b0000060 	adrp	x0, 40e000 <__stdio_ofl_lockptr+0xa68>
  401de0:	911f4000 	add	x0, x0, #0x7d0
  401de4:	f94013f5 	ldr	x21, [sp, #32]
  401de8:	b9400401 	ldr	w1, [x0, #4]
  401dec:	11000421 	add	w1, w1, #0x1
  401df0:	b9000401 	str	w1, [x0, #4]
  401df4:	a8dd7bfd 	ldp	x29, x30, [sp], #464
  401df8:	d65f03c0 	ret
  401dfc:	9100f3e0 	add	x0, sp, #0x3c
  401e00:	94000b10 	bl	404a40 <wait>
  401e04:	d1000683 	sub	x3, x20, #0x1
  401e08:	d2800021 	mov	x1, #0x1                   	// #1
  401e0c:	d503201f 	nop
  401e10:	910103e0 	add	x0, sp, #0x40
  401e14:	38616864 	ldrb	w4, [x3, x1]
  401e18:	8b010002 	add	x2, x0, x1
  401e1c:	385ff040 	ldurb	w0, [x2, #-1]
  401e20:	6b00009f 	cmp	w4, w0
  401e24:	54000221 	b.ne	401e68 <file_shared_with_fork_test+0x194>  // b.any
  401e28:	91000421 	add	x1, x1, #0x1
  401e2c:	f103243f 	cmp	x1, #0xc9
  401e30:	54ffff01 	b.ne	401e10 <file_shared_with_fork_test+0x13c>  // b.any
  401e34:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401e38:	91218000 	add	x0, x0, #0x860
  401e3c:	94000b84 	bl	404c4c <puts>
  401e40:	b0000062 	adrp	x2, 40e000 <__stdio_ofl_lockptr+0xa68>
  401e44:	911f4042 	add	x2, x2, #0x7d0
  401e48:	aa1403e0 	mov	x0, x20
  401e4c:	d2801901 	mov	x1, #0xc8                  	// #200
  401e50:	b9400443 	ldr	w3, [x2, #4]
  401e54:	11000463 	add	w3, w3, #0x1
  401e58:	b9000443 	str	w3, [x2, #4]
  401e5c:	94000a94 	bl	4048ac <__munmap>
  401e60:	a94153f3 	ldp	x19, x20, [sp, #16]
  401e64:	17ffffd8 	b	401dc4 <file_shared_with_fork_test+0xf0>
  401e68:	aa1403e0 	mov	x0, x20
  401e6c:	d2801901 	mov	x1, #0xc8                  	// #200
  401e70:	94000a8f 	bl	4048ac <__munmap>
  401e74:	3100041f 	cmn	w0, #0x1
  401e78:	54000600 	b.eq	401f38 <file_shared_with_fork_test+0x264>  // b.none
  401e7c:	2a1303e0 	mov	w0, w19
  401e80:	940013ab 	bl	406d2c <close>
  401e84:	f942eea0 	ldr	x0, [x21, #1496]
  401e88:	52800041 	mov	w1, #0x2                   	// #2
  401e8c:	94000a13 	bl	4046d8 <open>
  401e90:	2a0003f3 	mov	w19, w0
  401e94:	4f03e5e0 	movi	v0.16b, #0x6f
  401e98:	910423e1 	add	x1, sp, #0x108
  401e9c:	d2801902 	mov	x2, #0xc8                  	// #200
  401ea0:	ad0203e0 	stp	q0, q0, [sp, #64]
  401ea4:	3d801be0 	str	q0, [sp, #96]
  401ea8:	7d00e3e0 	str	h0, [sp, #112]
  401eac:	940013b0 	bl	406d6c <read>
  401eb0:	f103201f 	cmp	x0, #0xc8
  401eb4:	540003a1 	b.ne	401f28 <file_shared_with_fork_test+0x254>  // b.any
  401eb8:	f94087e1 	ldr	x1, [sp, #264]
  401ebc:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401ec0:	911ee000 	add	x0, x0, #0x7b8
  401ec4:	94000b42 	bl	404bcc <printf>
  401ec8:	d2800020 	mov	x0, #0x1                   	// #1
  401ecc:	d503201f 	nop
  401ed0:	910423e1 	add	x1, sp, #0x108
  401ed4:	8b000022 	add	x2, x1, x0
  401ed8:	910103e1 	add	x1, sp, #0x40
  401edc:	8b000021 	add	x1, x1, x0
  401ee0:	385ff042 	ldurb	w2, [x2, #-1]
  401ee4:	385ff021 	ldurb	w1, [x1, #-1]
  401ee8:	6b01005f 	cmp	w2, w1
  401eec:	540002e1 	b.ne	401f48 <file_shared_with_fork_test+0x274>  // b.any
  401ef0:	91000400 	add	x0, x0, #0x1
  401ef4:	f103241f 	cmp	x0, #0xc9
  401ef8:	54fffec1 	b.ne	401ed0 <file_shared_with_fork_test+0x1fc>  // b.any
  401efc:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401f00:	91214000 	add	x0, x0, #0x850
  401f04:	94000b52 	bl	404c4c <puts>
  401f08:	2a1303e0 	mov	w0, w19
  401f0c:	94001388 	bl	406d2c <close>
  401f10:	b0000061 	adrp	x1, 40e000 <__stdio_ofl_lockptr+0xa68>
  401f14:	a94153f3 	ldp	x19, x20, [sp, #16]
  401f18:	b947d020 	ldr	w0, [x1, #2000]
  401f1c:	11000400 	add	w0, w0, #0x1
  401f20:	b907d020 	str	w0, [x1, #2000]
  401f24:	17ffffa8 	b	401dc4 <file_shared_with_fork_test+0xf0>
  401f28:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401f2c:	911e8000 	add	x0, x0, #0x7a0
  401f30:	94000b47 	bl	404c4c <puts>
  401f34:	17ffff9e 	b	401dac <file_shared_with_fork_test+0xd8>
  401f38:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401f3c:	911e2000 	add	x0, x0, #0x788
  401f40:	94000b43 	bl	404c4c <puts>
  401f44:	17ffff9a 	b	401dac <file_shared_with_fork_test+0xd8>
  401f48:	3941cbe6 	ldrb	w6, [sp, #114]
  401f4c:	d0000040 	adrp	x0, 40b000 <_fini+0x780>
  401f50:	3941c7e5 	ldrb	w5, [sp, #113]
  401f54:	911f2000 	add	x0, x0, #0x7c8
  401f58:	394103e4 	ldrb	w4, [sp, #64]
  401f5c:	3944ebe3 	ldrb	w3, [sp, #314]
  401f60:	3944e7e2 	ldrb	w2, [sp, #313]
  401f64:	394423e1 	ldrb	w1, [sp, #264]
  401f68:	94000b19 	bl	404bcc <printf>
  401f6c:	17ffff90 	b	401dac <file_shared_with_fork_test+0xd8>
```

## uvm_copy2()を作成

- mmap-region部分のマッピングはコピーしない
- p->base -> p->sz, p->stksz -> USERTOPまでをコピー
- mmap-region部分はcopy_mmap_listでマッピング

```
init: starting sh
[1]pgdir_walk: failed
pte shuld exist: addr=0xa000
kern/console.c:283: kernel panic at cpu 1.
```

### uvm_copy2のバグ

- stkszはあくまでサイズなのでスタックの開始アドレスは`USERTOP - stksz`
- 結果は同じな上に終了せずスタック

```
# mmaptest2

[F-13] file backed shared mapping with fork test
buf2[0]=0xffff00003bb42000
[F-13] failed at strcmp 3: buf2[0, 49, 50]=[, o, a], buf=[o, o, a]

file_test:  ok: 0, ng: 1
anon_test:  ok: 0, ng: 0
other_test: ok: 0, ng: 0    // ここでスタック
```

### ret2アドレスの値が変更された時にブレークさせる

- `watch *0x600000001000` (4バイト単位)

```
(gdb) c
Continuing.
[Switching to Thread 1.2]

Thread 2 hit Hardware watchpoint 1: *0x600000001000

Old value = <unreadable>
New value = 1869573999  (= 0x6f6f6f6f)
0x0000000000401d5c in ?? ()
(gdb) c
Continuing.
[Switching to Thread 1.1]

Thread 1 hit Hardware watchpoint 1: *0x600000001000

Old value = 1869573999  (= 0x6f6f6f6f)
New value = 4395432     (= 0x4311a8)
0x000000000041a8dc in ?? ()
(gdb) bt
#0  0x000000000041a8dc in ?? ()
Backtrace stopped: previous frame identical to this frame (corrupt stack?)
(gdb) c
Continuing.
[Inferior 1 (process 1) exited normally]
```

- `watch *(char *)0x600000001000` (1バイト単位)

```
(gdb) c
Continuing.
[Switching to Thread 1.2]

Thread 2 hit Hardware watchpoint 1: *(char *)0x600000001000

Old value = <unreadable>
New value = 111 'o'
0x0000000000401d5c in ?? ()
(gdb) c
Continuing.
[Switching to Thread 1.1]

Thread 1 hit Hardware watchpoint 1: *(char *)0x600000001000

Old value = 111 'o'
New value = 168 '\250'
0x000000000041a8dc in ?? ()
```

- 2回目のストップ時にはすでに実行を終えている。
- 2回めのストップ時のアドレス0x000000000041a8dcはmmaptest2でもdashでもない

### mmap.cを変更中

- uvm_map()にpermを渡すようにしたがPTE_UDATA以外のpermを指定すると最初に`ls`を実行するとストールする。
- `/bin/ls`や`mmaptest2`を最初に実行すると動く。
- ただし、2つ目のコマンドがストールする
- PTE_UDATAにするとls, /bin/ls, mmaptest2が動くが、mmaptest2実行後はlsも/bin/lsもストールする
- mmaptestも同じ症状になった

```
# mmaptest
mmap_testスタート
[7] test mmap two files
- open mmap1 (3) RDWR/CREAT
- write 3: 12345
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000001000
- close 3
- unlink mmap1
- open mmap2 (3) RDWR/CREAT
- write 3: 67890
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000002000
- close 3
- unlink mmap2
- munmap PAGE: addr=0x600000001000
- munmap PAGE: addr=0x600000002000
[7] OK
mmap_test: Total: 1, OK: 1, NG: 0

fork_test starting
p1[PGSIZE]=A
p2[PGSIZE]=A
mismatch at 0, wanted 'A', got 0x0, addr=0x600000001000
mismatch at 1, wanted 'A', got 0x80, addr=0x600000001001
mismatch at 2, wanted 'A', got 0xbb, addr=0x600000001002
mismatch at 3, wanted 'A', got 0x3b, addr=0x600000001003
mismatch at 4, wanted 'A', got 0x0, addr=0x600000001004
mismatch at 5, wanted 'A', got 0x0, addr=0x600000001005
mismatch at 6, wanted 'A', got 0xff, addr=0x600000001006
mismatch at 7, wanted 'A', got 0xff, addr=0x600000001007
- fork parent v1(p1): ret: -8
mismatch at 0, wanted 'A', got 0x0, addr=0x600000003000
mismatch at 1, wanted 'A', got 0x90, addr=0x600000003001
mismatch at 2, wanted 'A', got 0xbb, addr=0x600000003002
mismatch at 3, wanted 'A', got 0x3b, addr=0x600000003003
mismatch at 4, wanted 'A', got 0x0, addr=0x600000003004
mismatch at 5, wanted 'A', got 0x0, addr=0x600000003005
mismatch at 6, wanted 'A', got 0xff, addr=0x600000003006
mismatch at 7, wanted 'A', got 0xff, addr=0x600000003007
- fork parent v1(p2): ret: -8
fork_test OK
mmaptest: all tests succeeded
```
