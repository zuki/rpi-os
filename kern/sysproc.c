#include "proc.h"
#include "trap.h"
#include "console.h"
#include "vm.h"
#include "syscall1.h"

#include <sys/mman.h>
#include <errno.h>
#include "signal.h"

long
sys_yield()
{
    yield();
    return 0;
}

/* From: man 2 brk

brk() は、データセグメントの末尾を addr で指定した値に設定する。

実際、muslにおけるSYS_brk()の呼び出し事例は以下の3種類で、いずれも引数はアドレス。
(ここではmallocngを使っているので実際は(3)のケースである)
 1: __syscall(SYS_brk, 0) : malloc/oldmalloc/malloc.c
 2: __syscall(SYS_brk, brk+req)==brk+req : malloc/oldmalloc/malloc.c
 3: #define brk(p) ((uintptr_t)__syscall(SYS_brk, p)) : malloc/mallocng/glue.h

また、manによれば、以下のようにある。ここではlinuxの実装に合わせた。
（mallocngがこの返り値を使用しているため)

返り値
    成功した場合、 brk() は 0 を返す。 エラーの場合には、-1 を返し、 errno
    に ENOMEM を設定する。

注意
   C ライブラリとカーネルの違い
       しかし、実際の Linux システムコールは、成功した場合、  プログラムの新しい
       ブレークを返す。 失敗した場合、このシステムコールは現在のブレークを返す。
       glibc ラッパー関数は同様の働きをし (すなわち、新しいブレークが addr より
       小さいかどうかをチェックし)、 上で説明した 0 と -1 という返り値を返す。

以下はmmapを使わない場合。プロセスメモリにmmapを使う場合は修正が必要。
*/
size_t
sys_brk()
{
    struct proc *p = thisproc();
    size_t sz, newsz, oldsz = p->sz;

    //panic("sys_brk: unimplemented. ");

    if (argu64(0, &newsz) < 0)
        return oldsz;

    trace("name %s: 0x%llx to 0x%llx", p->name, oldsz, newsz);

    if (newsz == 0)
        return oldsz;

    if (newsz < oldsz) {
        p->sz = uvm_dealloc(p->pgdir, p->base, oldsz, newsz);
    } else {
        sz = uvm_alloc(p->pgdir, p->base, p->stksz, oldsz, newsz);
        if (sz == 0)
            return oldsz;
        p->sz = sz;
    }
    return p->sz;
}

size_t
sys_mmap()
{
    void *addr;
    size_t len, off;
    int prot, flags, fd;
    if (argu64(0, (uint64_t *) & addr) < 0 ||
        argu64(1, &len) < 0 ||
        argint(2, &prot) < 0 ||
        argint(3, &flags) < 0 || argint(4, &fd) < 0 || argu64(5, &off) < 0)
        return -EINVAL;

    if ((flags & MAP_PRIVATE) == 0 || (flags & MAP_ANON) == 0 || fd != -1
        || off != 0) {
        warn("non-private mmap unimplemented: flags 0x%x, fd %d, off %d",
             flags, fd, off);
        return -EPERM;
    }

    if (addr) {
        if (prot != PROT_NONE) {
            warn("mmap unimplemented");
            return -EPERM;
        }
        trace("map none at 0x%p", addr);
        return (size_t)addr;
    } else {
        if (prot != (PROT_READ | PROT_WRITE)) {
            warn("non-rw unimplemented");
            return -EPERM;
        }
        //panic("unimplemented. ");
        return -EPERM;
    }
}

long
sys_clone()
{
    void *childstk;
    uint64_t flag;
    if (argu64(0, &flag) < 0 || argu64(1, (uint64_t *) & childstk) < 0)
        return -1;
    trace("flags 0x%llx, child stack 0x%p", flag, childstk);
    if (flag != 17) {
        warn("flags other than SIGCHLD are not supported");
        return -1;
    }
    return fork();
}


long
sys_wait4()
{
    int pid, opt;
    int *wstatus;
    void *rusage;
    if (argint(0, &pid) < 0 ||
        argu64(1, (uint64_t *) & wstatus) < 0 ||
        argint(2, &opt) < 0 || argu64(3, (uint64_t *) & rusage) < 0)
        return -1;

    // FIXME:
    if (pid != -1 || opt != 0 || rusage != 0) {
        warn("unimplemented. pid %d, wstatus 0x%p, opt 0x%x, rusage 0x%p",
             pid, wstatus, opt, rusage);
        return -1;
    }

    return wait(pid, wstatus);
}

long
sys_kill()
{
    int pid, sig;

    if (argint(0, &pid) < 0 || argint(1, &sig) < 0)
        return -EINVAL;

    if (sig < 1 || sig >= NSIG)
        return -EINVAL;

    trace("pid=%d, sig=%d", pid, sig);

    return kill(pid, sig);
}

long
sys_rt_sigsuspend()
{
    sigset_t *mask;

    if (argptr(0, &mask, sizeof(sigset_t)) < 0)
        return -EINVAL;

    trace("mask=%lld", *mask);
    if (!in_user(mask, sizeof(sigset_t)))
        return -EFAULT;

    return sigsuspend(mask);
}

long
sys_rt_sigaction()
{
    int sig;
    struct k_sigaction *act, *oldact;

    if (argint(0, &sig) < 0 || argu64(1, (uint64_t *)&act) < 0
     || argu64(2, (uint64_t *)&oldact) < 0)
        return -EINVAL;

    trace("sig=%d, act=0x%llx, oldact=0x%llx", sig, act, oldact);

    if (sig < 1 || sig >= NSIG || sig == SIGSTOP || sig == SIGKILL)
        return -EINVAL;

    if ((act && !in_user(act, sizeof(struct k_sigaction)))
     || (oldact && !in_user(oldact, sizeof(struct k_sigaction))))
        return -EFAULT;

    return sigaction(sig, act, oldact);
}

long
sys_rt_sigpending()
{
    sigset_t *pending;

    if (argu64(0, (uint64_t *)&pending) < 0)
        return -EINVAL;

    trace("pendig=0x%llx", pending);
    if (!in_user(pending, sizeof(sigset_t)))
        return -EFAULT;

    return sigpending(pending);
}

long
sys_rt_sigprocmask()
{
    int how;
    sigset_t *set, *oldset;
    size_t size;

    if (argint(0, &how) < 0 || argu64(1, (uint64_t *)&set) < 0
     || argu64(2, (uint64_t *)&oldset) < 0 || argu64(3, (uint64_t *)&size) < 0)
        return -EINVAL;

    trace("how=%d, *act=0x%llx, oldact=0x%llx, size=%lld", how, *set, oldset, size);

    return sigprocmask(how, set, oldset, size);
}

long
sys_rt_sigreturn()
{
    return sigreturn();
}
