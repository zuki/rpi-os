#include "linux/errno.h"
#include "proc.h"
#include "trap.h"
#include "console.h"
#include "string.h"
#include "vm.h"
#include "syscall1.h"
#include "linux/mman.h"
#include "mmap.h"
#include "linux/signal.h"
#include "linux/ppoll.h"
#include "linux/capability.h"
#include "linux/resources.h"

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

    debug("name %s: 0x%llx to 0x%llx", p->name, oldsz, newsz);

    if (newsz == 0)
        return oldsz;

    if (newsz < oldsz) {
        p->sz = uvm_dealloc(p->pgdir, p->base, oldsz, newsz);
    } else {
        sz = uvm_alloc(p->pgdir, p->base, p->stksz, oldsz, newsz);
        if (sz == 0) {
            warn("uvm_alloc failed");
            return oldsz;
        }
        p->sz = sz;
    }
    return p->sz;
}

long
sys_mmap()
{
    void *addr;
    size_t length, offset;
    int prot, flags, fd;
    struct file *f;

    if (argu64(0, (uint64_t *)&addr) < 0 ||
        argu64(1, &length) < 0 ||
        argint(2, &prot) < 0 ||
        argint(3, &flags) < 0 || argint(4, &fd) < 0 || argu64(5, &offset) < 0)
        return -EINVAL;

    trace("addr=0x%llx, length=0x%lld, prot=0x%x, flags=0x%x, offset=0x%lld", addr, length, prot, flags, offset);

    if (flags & MAP_ANONYMOUS) {
        if (fd != -1) return -EINVAL;
        f = NULL;
    } else {
        if (fd < 0 || fd >= NOFILE) return -EBADF;
        if ((f = thisproc()->ofile[fd]) == 0) return -EBADF;
    }

    if ((flags & (MAP_PRIVATE | MAP_SHARED)) == 0) {
        warn("invalid flags: 0x%x", flags);
        return -EINVAL;
    }

    if ((ssize_t)length <= 0 || (ssize_t)offset < 0) {
        warn("invalid length: %lld or offset: %lld", length, offset);
        return -EINVAL;
    }

    // MAP_FIXEDの場合、addrが指定されていなければならない
    if ((flags & MAP_FIXED) && addr == NULL) {
        warn("MAP_FIXED and addr is NULL");
        return -EINVAL;
    }

    // バックにあるファイルはreadableでなければならない
    if (!(flags & MAP_ANONYMOUS) && !f->readable) {
        warn("file is not readable");
        return -EACCES;
    }

    // MAP_SHAREかつPROT_WRITEの場合はバックにあるファイルがwritableでなければならない
    if (!(flags & MAP_ANONYMOUS) && (flags & MAP_SHARED)
     && (prot & PROT_WRITE) && !f->writable) {
        warn("file is not writable");
        return -EACCES;
    }
/*  たぶん、不要
    if (flags & MAP_ANONYMOUS)
        length = ROUNDUP(length, PGSIZE);
*/
    return mmap(addr, length, prot, flags, f, offset);
}

long
sys_munmap()
{
    void *addr;
    size_t length;

    if (argu64(0, (uint64_t *)&addr) < 0 || argu64(1, &length) < 0)
        return -EINVAL;

    debug("addr: 0x%llx, length: 0x%llx", addr, length);

    return munmap(addr, length);
}

long
sys_msync()
{
    void *addr;
    size_t length;
    int flags;

    if (argu64(0, (uint64_t *)&addr) < 0 || argu64(1, &length) < 0
     || argint(2, &flags) < 0)
        return -EINVAL;

    return msync(addr, length, flags);

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
    int pid, options;
    int *wstatus;
    struct rusage *rusage;
    if (argint(0, &pid) < 0 || argptr(1, (char **)&wstatus, sizeof(int)) < 0
     || argint(2, &options) < 0
     || argptr(3, (char **)&rusage, sizeof(struct rusage)) < 0)
        return -EINVAL;

    return wait4(pid, wstatus, options, rusage);
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
sys_tkill()
{
    return 0;
}

long
sys_rt_sigsuspend()
{
    sigset_t *mask;

    if (argptr(0, (char **)&mask, sizeof(sigset_t)) < 0)
        return -EINVAL;

    trace("p.mask: 0x%llx, mask: 0x%llx", thisproc()->signal.mask, *mask);

    return sigsuspend(mask);
}

long
sys_rt_sigaction()
{
    int sig;
    struct k_sigaction *act, *oldact;

    if (argint(0, &sig) < 0 || argptr(1, (char **)&act, sizeof(struct k_sigaction)) < 0
     || argptr(2, (char **)&oldact, sizeof(struct k_sigaction)) < 0)
        return -EINVAL;

    trace("sig=%d, act=0x%llx, oldact=0x%llx", sig, act, oldact);

    if (sig < 1 || sig >= NSIG || sig == SIGSTOP || sig == SIGKILL)
        return -EINVAL;

    return sigaction(sig, act, oldact);
}

long
sys_rt_sigpending()
{
    sigset_t *pending;

    if (argptr(0, (char **)&pending, sizeof(sigset_t)) < 0)
        return -EINVAL;

    trace("pendig: 0x%p", pending);

    return sigpending(pending);
}

long
sys_rt_sigprocmask()
{
    int how;
    sigset_t *set, *oldset;
    size_t size;

    if (argint(0, &how) < 0 || argptr(1, (char **)&set, sizeof(sigset_t)) < 0
     || argptr(2, (char **)&oldset, sizeof(sigset_t)) < 0 || argu64(3, (uint64_t *)&size) < 0)
        return -EINVAL;

    trace("[%d] how=%d, *set=0x%llx, oldset=0x%p, size=%lld", thisproc()->pid, how, set ? *set : 0, oldset, size);

    return sigprocmask(how, set, oldset, size);
}

long
sys_rt_sigreturn()
{
    return sigreturn();
}

//FIXME: ちゃんと実装する
long sys_ppoll() {
    struct pollfd *fds;
    nfds_t nfds;
    // TODO: 以下2変数の実装
    //struct timespec *timeout_ts;
    //sigset_t *sigmask;

    if (argu64(1, &nfds) < 0
     || argptr(0, (char **)&fds, nfds * sizeof(struct pollfd)) < 0)
        return -EINVAL;
    trace("fds: 0x%p, nfds: %lld", fds, nfds);
    return ppoll(fds, nfds);
}

mode_t
sys_umask()
{
    mode_t umask;
    mode_t oumask = thisproc()->umask;

    if (argint(0, (int *)&umask) < 0)
        return -EINVAL;

    thisproc()->umask = umask & S_IRWXUGO;
    return oumask;
}

static inline void
cap_emulate_setxuid(int old_ruid, int old_euid, int old_suid)
{
    struct proc *p = thisproc();

    if ((old_ruid == 0 || old_euid == 0 || old_suid == 0) &&
        (p->uid != 0 && p->euid != 0 && p->suid != 0)) {
        cap_clear(p->cap_permitted);
        cap_clear(p->cap_effective);
    }
    if (old_euid == 0 && p->euid != 0) {
        cap_clear(p->cap_effective);
    }
    if (old_euid != 0 && p->euid == 0) {
        p->cap_effective = p->cap_permitted;
    }
}

/* ユーザIDを設定する */
long
sys_setuid()
{
    struct proc *p = thisproc();
    uid_t uid, old_ruid, old_euid, old_suid, new_ruid, new_suid;

    if (argint(0, (int *)&uid) < 0)
        return -EINVAL;

    old_euid = p->euid;
    old_ruid = new_ruid = p->uid;
    new_suid = old_suid = p->suid;

    debug("uid: %d, old: euid=%d ruid=%d suid=%d, new: ruid=%d suid=%d",
        uid, old_euid, old_ruid, old_suid, new_ruid, new_suid);
    debug("p[%d] cap_effective=%d", p->pid, p->cap_effective);

    if (capable(CAP_SETUID)) {
        if (uid != old_ruid) {
            p->uid = uid;
            new_suid = uid;
        } else if ((uid != p->uid) && (uid != new_suid)) {
            return -EPERM;
        }
    }

    if (old_euid != uid) disb();
    p->fsuid = p->euid = uid;
    p->suid = new_suid;

    cap_emulate_setxuid(old_ruid, old_euid, old_suid);

    return 0;
}

/* 実 (real)ユーザIDと実効 (effective)ユーザIDを設定する */
long
sys_setreuid()
{
    struct proc *p = thisproc();
    uid_t ruid, euid, old_ruid, old_euid, old_suid, new_ruid, new_euid;

    if (argint(0, (int *)&ruid) < 0 || argint(1, (int *)&euid) < 0)
        return -EINVAL;

    new_ruid = old_ruid = p->uid;
    new_euid = old_euid = p->euid;
    old_suid = p->suid;

    if (ruid != (uid_t)-1) {
        new_ruid = ruid;
        if ((old_ruid != ruid) && (p->euid != ruid) && !capable(CAP_SETUID))
            return -EPERM;
    }

    if (euid != (uid_t)-1) {
        new_euid = euid;
        if ((old_euid != euid) && (p->suid != euid) && !capable(CAP_SETUID))
            return -EPERM;
    }

    if (new_ruid != old_ruid)
        p->uid = ruid;

    if (new_euid != old_euid) disb();
    p->fsuid = p->euid = new_euid;

    if (ruid != (uid_t)-1 || (euid != (uid_t)-1 && euid != old_ruid))
        p->suid = p->euid;
    p->fsuid = p->euid;

    cap_emulate_setxuid(old_ruid, old_euid, old_suid);

    return 0;
}

/* ユーザの実ID、実効ID、保存IDを設定する */
long
sys_setresuid()
{
    struct proc *p = thisproc();
    uid_t ruid, euid, suid;
    uid_t old_ruid = p->uid, old_euid = p->euid, old_suid= p->suid;

    if (argint(0, (int *)&ruid) < 0 || argint(1, (int *)&euid) < 0
     || argint(2, (int *)&suid) < 0)
        return -EINVAL;

    if (!capable(CAP_SETUID)) {
        if ((ruid != (uid_t)-1) && (ruid != p->uid) &&
            (ruid != p->euid) && (ruid != p->suid))
            return -EPERM;
        if ((euid != (uid_t)-1) && (euid != p->uid) &&
            (euid != p->euid) && (euid != p->suid))
            return -EPERM;
        if ((suid != (uid_t)-1) && (suid != p->uid) &&
            (suid != p->euid) && (suid != p->suid))
            return -EPERM;
    }

    if (ruid != (uid_t)-1)
        p->uid = ruid;

    if (euid != (uid_t)-1) {
        if (euid != p->euid) disb();
        p->euid = euid;
        p->fsuid = euid;
    }
    if (suid != (uid_t)-1)
        p->suid = suid;

    cap_emulate_setxuid(old_ruid, old_euid, old_suid);

    return 0;
}

/* ファイルシステムのチェックに用いられるユーザIDを設定する */
long
sys_setfsuid()
{
    struct proc *p = thisproc();

    uid_t fsuid, old_fsuid = p->fsuid;

    if (argint(0, (int *)&fsuid) < 0)
        return -EINVAL;

    if (fsuid == p->uid || fsuid == p->euid || fsuid == p->suid
     || fsuid == p->fsuid || capable(CAP_SETUID)) {
         if (fsuid != old_fsuid) disb();
         p->fsuid = fsuid;
    }

    if (old_fsuid == 0 && p->fsuid != 0)
        cap_t(p->cap_effective) &= ~CAP_FS_MASK;
    if (old_fsuid != 0 && p->fsuid == 0)
        cap_t(p->cap_effective) |= (cap_t(p->cap_permitted) & CAP_FS_MASK);

    return old_fsuid;
}

/* グループIDを設定する */
long
sys_setgid()
{
    struct proc *p = thisproc();
    gid_t gid, old_egid = p->egid;

    if (argint(0, (int *)&gid) < 0)
        return -EINVAL;

    if (capable(CAP_SETGID)) {
        if (old_egid != gid) disb();
        p->gid = p->egid = p->sgid = gid;
    } else if ((gid == p->gid) || (gid == p->sgid)) {
        if (old_egid != gid) disb();
        p->egid = gid;
    } else {
        return -EPERM;
    }

    return 0;
}

/* 実グループIDと実効グループIDを設定する */
long
sys_setregid()
{
    struct proc *p = thisproc();
    gid_t rgid, egid;
    gid_t old_rgid = p->gid, old_egid = p->egid;
    gid_t new_rgid, new_egid;

    if (argint(0, (int *)&rgid) < 0 || argint(1, (int *)&egid) < 0)
        return -EINVAL;

    new_rgid = old_rgid;
    new_egid = old_egid;

    if (rgid != (gid_t)-1) {
        if ((old_rgid == rgid) || (p->egid == rgid) || capable(CAP_SETGID))
            new_rgid = rgid;
        else
            return -EPERM;
    }
    if (egid != (gid_t)-1) {
        if ((old_rgid == egid) || (p->egid == egid) || (p->sgid == egid)
          || capable(CAP_SETGID))
            new_egid = egid;
        else
            return -EPERM;
    }

    if (new_egid != old_egid) disb();
    if (rgid != (gid_t)-1 || (egid != (gid_t)-1 && egid != old_rgid))
        p->sgid = new_egid;
    p->fsgid = new_egid;
    p->egid = new_egid;
    p->gid = new_rgid;

    return 0;
}

/* 実グループID、実効グループID、保存グループIDを設定する */
long
sys_setresgid()
{
    struct proc *p = thisproc();
    gid_t rgid, egid, sgid;

    if (argint(0, (int *)&rgid) < 0 || argint(1, (int *)&egid) < 0
     || argint(2, (int *)&sgid) < 0)
        return -EINVAL;

    if (!capable(CAP_SETGID)) {
        if ((rgid != (gid_t)-1) && (rgid != p->gid) &&
            (rgid != p->egid) && (rgid != p->sgid))
            return -EPERM;
        if ((egid != (gid_t)-1) && (egid != p->gid) &&
            (egid != p->egid) && (egid != p->sgid))
            return -EPERM;
        if ((sgid != (gid_t)-1) && (sgid != p->gid) &&
            (sgid != p->egid) && (sgid != p->sgid))
            return -EPERM;
    }

    if (egid != (gid_t)-1) {
        if (egid != p->egid) disb();
        p->egid = egid;
        p->fsgid = egid;
    }

    if (rgid != (gid_t)-1)
        p->gid = rgid;
    if (sgid != (gid_t)-1)
        p->sgid = sgid;

    return 0;
}

/* ファイルシステムのチェックに用いられるグループIDを設定する */
long
sys_setfsgid()
{
    struct proc *p = thisproc();
    gid_t fsgid, old_fsgid = p->fsgid;

    if (argint(0, (int *)&fsgid) < 0)
        return -EINVAL;

    if (fsgid == p->gid || fsgid == p->egid || fsgid == p->sgid
     || fsgid == p->fsgid || capable(CAP_SETGID)) {
         if (fsgid != old_fsgid) disb();
         p->fsgid = fsgid;
    }
    return old_fsgid;
}

/* 実ユーザID、実効ユーザID、保存ユーザIDを取得する */
long
sys_getresuid()
{
    struct proc *p = thisproc();
    uid_t *ruid, *euid, *suid;

    if (argptr(0, (char **)&ruid, sizeof(uid_t)) < 0
     || argptr(1, (char **)&euid, sizeof(uid_t)) < 0
     || argptr(2, (char **)&suid, sizeof(uid_t)) < 0)
        return -EINVAL;

    *ruid = p->uid;
    *euid = p->euid;
    *suid = p->suid;

    return 0;
}

/* 実グループID、実効グループID、保存グループIDを取得する */
long
sys_getresgid()
{
    struct proc *p = thisproc();
    gid_t *rgid, *egid, *sgid;

    if (argptr(0, (char **)&rgid, sizeof(gid_t)) < 0
     || argptr(1, (char **)&egid, sizeof(gid_t)) < 0
     || argptr(2, (char **)&sgid, sizeof(gid_t)) < 0)
        return -EINVAL;

    *rgid = p->gid;
    *egid = p->egid;
    *sgid = p->sgid;

    return 0;
}

long
sys_getuid()
{
    return thisproc()->uid;
}

long
sys_geteuid()
{
    return thisproc()->euid;
}

long
sys_getgid()
{
    return thisproc()->gid;
}

long
sys_getegid()
{
    return thisproc()->egid;
}

/* 補助グループIDのリストを取得する */
long
sys_getgroups()
{
    struct proc *p = thisproc();
    size_t size;
    gid_t *list;
    int ngroups = p->ngroups;

    if (argu64(0, &size) < 0) return -EINVAL;
    if (size < 0) return -EINVAL;
    if (size == 0) return ngroups;
    if (ngroups > size) return -EINVAL;

    if (argptr(1, (char **)&list, sizeof(gid_t) * size) < 0)
        return -EINVAL;

    memmove(list, p->groups, sizeof(gid_t) * ngroups);
    return ngroups;
}

/* 補助グループIDのリストを設定する */
long
sys_setgroups()
{
    struct proc *p = thisproc();
    size_t size;
    gid_t *list;

    if (argu64(0, &size) < 0) return -EINVAL;
    if (size < 0) return -EINVAL;
    if (size > NGROUPS) return -EINVAL;
    if (!capable(CAP_SETGID)) return -EPERM;

    if (argptr(1, (char **)&list, sizeof(gid_t) * size) < 0)
        return -EINVAL;

    if (size == 0 && list == NULL) {
        memset(p->groups, 0, sizeof(gid_t) * p->ngroups);
        p->ngroups = 0;
        return 0;
    }

    if (size > 0 && list != NULL) {
        memmove(p->groups, list, sizeof(gid_t) * size);
        p->ngroups = size;
    }

    return 0;
}

// FIXME: use pid instead of tid since we don't have threads :)
long sys_set_tid_address() {
    trace("set_tid_address: name '%s'", thisproc()->name);
    return thisproc()->pid;
}

long sys_getpid() {
    return thisproc()->pid;
}

long sys_gettid() {
    trace("gettid: name '%s'", thisproc()->name);
    return thisproc()->pid;
}

long
sys_getppid()
{
    return thisproc()->parent->pid;
}

// FIXME: exit_group should kill every thread in the current thread group.
long sys_exit_group() {
    trace("sys_exit_group: '%s' exit with code %d", thisproc()->name, thisproc()->tf->x[0]);
    exit(thisproc()->tf->x[0]);
    return 0;
}

long sys_exit() {
    trace("sys_exit: '%s' exit with code %d", thisproc()->name, thisproc()->tf->x[0]);
    exit(thisproc()->tf->x[0]);
    return 0;
}

long
sys_getpgid()
{
    pid_t pid;

    if (argint(0, &pid) < 0)
        return -EINVAL;

    return getpgid(pid);
}

long
sys_setpgid()
{
    pid_t pid, pgid;

    if (argint(0, &pid) < 0 || argint(1, &pgid) < 0)
        return -EINVAL;

    return setpgid(pid, pgid);
}
