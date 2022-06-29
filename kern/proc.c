#include "proc.h"
#include "linux/errno.h"

#include "string.h"
#include "types.h"
#include "memlayout.h"
#include "list.h"
#include "console.h"
#include "mm.h"
#include "vm.h"
#include "spinlock.h"
#include "mmap.h"
#include "dev.h"
#include "debug.h"
#include "file.h"
#include "log.h"
#include "string.h"
#include "linux/signal.h"
#include "linux/ppoll.h"
#include "linux/resources.h"
#include "linux/wait.h"

extern void trapret();
extern void swtch(struct context **old, struct context *new);

static void forkret();
static void idle_init();

#define SQSIZE  0x100           /* Must be power of 2. */
#define HASH(x) ((((uint64_t)(x)) >> 5) & (SQSIZE - 1))

struct cpu cpu[NCPU];

struct {
    struct proc proc[NPROC];
    struct list_head slpque[SQSIZE];
    struct list_head sched_que;
    struct spinlock lock;
} ptable;

struct _q {
    struct spinlock lock;
    struct spinlock siglock;
} q;

struct proc *initproc;
static pid_t pid = 0;

void
proc_init()
{
    list_init(&ptable.sched_que);
    for (int i = 0; i < SQSIZE; i++)
        list_init(&ptable.slpque[i]);
}

// TODO: use kmalloc
/*
 * Look in the process table for an UNUSED proc.
 * If found, change state to EMBRYO and initialize
 * state required to run in the kernel.
 * Otherwise return 0.
 */
static struct proc *
proc_alloc()
{
    struct proc *p;
    int found = 0;

    acquire(&ptable.lock);
    for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
        if (p->state == UNUSED) {
            memset(p, 0, sizeof(*p));
            found = 1;
            break;
        }
    }

    if (!found || !(p->kstack = kalloc())) {
        release(&ptable.lock);
        return 0;
    }

    p->pid = ++pid;
    p->state = EMBRYO;
    release(&ptable.lock);

    p->name[0] = 0;

    void *sp = p->kstack + PGSIZE;
    assert(sizeof(*p->tf) == 19 * 16 && sizeof(*p->context) == 7 * 16);

    sp -= sizeof(*p->tf);
    p->tf = sp;
    /* No user stack for init process. */
    p->tf->spsr = p->tf->sp = 0;

    sp -= sizeof(*p->context);
    p->context = sp;
    p->context->lr0 = (uint64_t) forkret;
    p->context->lr = (uint64_t) trapret;

    p->paused = 0;

    list_init(&p->child);

    return p;
}

static struct proc *
proc_initx(char *name, char *code, size_t len)
{
    struct proc *p = proc_alloc();
    void *va = kalloc();
    assert(p && va);

    p->pgdir = vm_init();
    assert(p->pgdir);

    int ret = uvm_map(p->pgdir, 0, PGSIZE, V2P(va));
    assert(ret == 0);

    memmove(va, code, len);
    assert(len <= PGSIZE);

    // Flush dcache to memory so that icache can retrieve the correct one.
    dccivac(va, len);

    p->stksz = 0;
    p->sz = PGSIZE;
    p->base = 0;

    p->pgid = p->sid = p->pid;
    p->fdflag = 0;
    p->umask = 0002;
    p->uid = p->euid = p->suid = p->fsuid = 0;
    p->gid = p->egid = p->sgid = p->fsgid = 0;
    p->ngroups = 0;
    memset(p->groups, 0, sizeof(gid_t)*NGROUPS);

    p->nregions = 0;
    p->regions = 0;
    initlock(p->regions_lock);

    p->tf->elr = 0;

    safestrcpy(p->name, name, sizeof(p->name));
    return p;
}

/* Initialize per-cpu idle process. */
static void
idle_init()
{
    extern char ispin[], eicode[];
    thiscpu()->idle = proc_initx("idle", ispin, (size_t)(eicode - ispin));
}

/* Set up the first user process. */
void
user_init()
{
    extern char icode[], eicode[];
    struct proc *p = proc_initx("icode", icode, (size_t)(eicode - icode));
    p->cwd = namei("/");
    assert(p->cwd);

    acquire(&ptable.lock);
    list_push_back(&ptable.sched_que, &p->link);
    release(&ptable.lock);
}

/*
 * Per-CPU process scheduler.
 * Each CPU calls scheduler() after setting itself up.
 * Scheduler never returns. It loops, doing:
 * - choose a process to run
 * - swtch to start running that process
 * - eventually that process transfers control
 *   via swtch back to the scheduler.
 */
void
scheduler()
{
    idle_init();
    for (struct proc * p;;) {
        acquire(&ptable.lock);
        struct list_head *head = &ptable.sched_que;
        if (list_empty(head)) {
            p = thiscpu()->idle;
        } else {
            p = container_of(list_front(head), struct proc, link);
            list_pop_front(head);
        }
        uvm_switch(p->pgdir);
        thiscpu()->proc = p;
        swtch(&thiscpu()->scheduler, p->context);
        release(&ptable.lock);
    }
}


/*
 * A fork child's very first scheduling by scheduler()
 * will swtch here. "Return" to user space.
 */
static void
forkret()
{
    static int first = 1;
    if (first && thisproc() != thiscpu()->idle) {
        first = 0;
        release(&ptable.lock);

        dev_init();
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    } else {
        release(&ptable.lock);
    }
    trace("proc '%s'(%d)", thisproc()->name, thisproc()->pid);
}

/* Give up CPU. */
void
yield()
{
    struct proc *p = thisproc();
    acquire(&ptable.lock);
    if (p != thiscpu()->idle)
        list_push_back(&ptable.sched_que, &p->link);
    p->state = RUNNABLE;
    swtch(&p->context, thiscpu()->scheduler);
    p->state = RUNNING;
    release(&ptable.lock);
}

/*
 * Atomically release lock and sleep on chan.
 * Reacquires lock when awakened.
 */
void
sleep(void *chan, struct spinlock *lk)
{
    struct proc *p = thisproc();
    int i = HASH(chan);
    assert(i < SQSIZE);
    assert(p != thiscpu()->idle);

    if (lk != &ptable.lock) {
        acquire(&ptable.lock);
        release(lk);
    }

    p->chan = chan;
    list_push_back(&ptable.slpque[i], &p->link);

    p->state = SLEEPING;
    swtch(&thisproc()->context, thiscpu()->scheduler);
    trace("'%s'(%d) wakeup lk=0x%p", p->name, p->pid, lk);
    p->state = RUNNING;

    if (lk != &ptable.lock) {
        release(&ptable.lock);
        acquire(lk);
    }
}

/*
 * Wake up all processes sleeping on chan.
 * The ptable lock must be held.
 */
static void
wakeup1(void *chan)
{
    struct list_head *q = &ptable.slpque[HASH(chan)];
    struct proc *p, *np;

    LIST_FOREACH_ENTRY_SAFE(p, np, q, link) {
        if (p->chan == chan) {
            list_drop(&p->link);
            list_push_back(&ptable.sched_que, &p->link);
            p->state = RUNNABLE;
        }
    }
}

/* Wake up all processes sleeping on chan. */
void
wakeup(void *chan)
{
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

/*
 * Create a new process copying p as the parent.
 * Sets up stack to return as if from system call.
 * Caller must set state of returned proc to RUNNABLE.
 */
int
fork()
{
    struct proc *cp = thisproc();
    struct proc *np = proc_alloc();
    long error;

    if (np == 0) {
        debug("proc_alloc returns null");
        return -ENOMEM;
    }

    if ((np->pgdir = uvm_copy(cp->pgdir)) == 0) {
        kfree(np->kstack);

        acquire(&ptable.lock);
        np->state = UNUSED;
        release(&ptable.lock);

        debug("uvm_copy returns null");
        return -ENOMEM;
    }

    if (cp->nregions != 0) {
        if ((error = copy_mmap_list(cp, np)) < 0) {
            kfree(np->kstack);
            acquire(&ptable.lock);
            np->state = UNUSED;
            release(&ptable.lock);
            debug("failed copy_mmap_list");
            return error;
        }
    }

    np->parent = cp;

    np->base = cp->base;
    np->sz = cp->sz;
    np->stksz = cp->stksz;

    memmove(np->tf, cp->tf, sizeof(*np->tf));

    // Fork returns 0 in the child.
    np->tf->x[0] = 0;

    for (int i = 0; i < NOFILE; i++)
        if (cp->ofile[i])
            np->ofile[i] = filedup(cp->ofile[i]);
    np->cwd = idup(cp->cwd);
    np->pgid = cp->pgid;
    np->sid = cp->sid;
    np->fdflag = cp->fdflag;
    np->uid = cp->uid;
    np->euid = cp->euid;
    np->suid = cp->suid;
    np->fsuid = cp->fsuid;
    np->gid = cp->gid;
    np->egid = cp->egid;
    np->sgid = cp->sgid;
    np->fsgid = cp->fsgid;
    np->ngroups = cp->ngroups;
    memmove(np->groups, cp->groups, sizeof(gid_t) * cp->ngroups);

    memmove(&np->signal, &cp->signal, sizeof(struct signal));

    int pid = np->pid;

    acquire(&ptable.lock);
    list_push_back(&cp->child, &np->clink);
    list_push_back(&ptable.sched_que, &np->link);
    np->state = RUNNABLE;
    release(&ptable.lock);

    trace("'%s'(%d) fork '%s'(%d)", cp->name, cp->pid, np->name, np->pid);

    return pid;
}


/*
 * Wait for a child process to exit and return its pid.
 * Return -1 if this process has no children.
 */
int
wait4(pid_t pid, int *status, int options, struct rusage *ru)
{
    struct proc *cp = thisproc();
    struct list_head *que = &cp->child;
    struct proc *p, *np;

    acquire(&ptable.lock);
    while (!list_empty(que)) {
        LIST_FOREACH_ENTRY_SAFE(p, np, que, clink) {
            if (p->parent != cp) continue;
            if (pid > 0) {
                if (p->pid != pid)
                    continue;
            } else if (pid == 0) {
                if (p->pgid != cp->pgid)
                    continue;
            } else if (pid != -1) {
                if (p->pgid != -pid)
                    continue;
            }
            if (p->state == ZOMBIE
             || (options & WUNTRACED && p->state == SLEEPING)
             || (options & WNOHANG)) {
                //assert(p->parent == cp);

                if (status) *status = p->xstate;
                if (ru) memset(ru, 0, sizeof(struct rusage));

                list_drop(&p->clink);

                kfree(p->kstack);
                vm_free(p->pgdir);
                p->state = UNUSED;

                int pid = p->pid;
                release(&ptable.lock);
                return pid;
            }
        }
        sleep(cp, &ptable.lock);
    }
    release(&ptable.lock);
    return -ECHILD;
}

/*
 * Exit the current process.  Does not return.
 * An exited process remains in the zombie state
 * until its parent calls wait() to find out it exited.
 */
void
exit(int err)
{
    struct proc *cp = thisproc();
    if (cp == initproc)
        panic("init exit");

    if (err) {
        info("exit: pid %d, name %s, err %d", cp->pid, cp->name, err);
    }

    // Close all open files.
    for (int fd = 0; fd < NOFILE; fd++) {
        if (cp->ofile[fd]) {
            fileclose(cp->ofile[fd]);
            cp->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(cp->cwd);
    end_op();
    cp->cwd = 0;

    acquire(&ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(cp->parent);

    // Pass abandoned children to init.
    struct list_head *q = &cp->child;
    struct proc *p, *np;
    LIST_FOREACH_ENTRY_SAFE(p, np, q, clink) {
        assert(p->parent == cp);
        p->parent = initproc;

        list_drop(&p->clink);
        list_push_back(&initproc->child, &p->clink);
        if (p->state == ZOMBIE)
            wakeup1(initproc);
    }
    assert(list_empty(q));

    // Jump into the scheduler, never to return.
    cp->xstate = err & 0xff;
    cp->state = ZOMBIE;

    swtch(&cp->context, thiscpu()->scheduler);
    panic("zombie exit");
}

/*
 * Print a process listing to console. For debugging.
 * Runs when user types ^P on console.
 */
void
procdump()
{
    static char *states[] = {
        [UNUSED] "unused",
        [EMBRYO] "embryo",
        [SLEEPING] "sleep ",
        [RUNNABLE] "runble",
        [RUNNING] "run   ",
        [ZOMBIE] "zombie"
    };
    struct proc *p;

    // Donot acquire ptable.lock to avoid deadlock
    // acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->parent)
            cprintf("%d %s %s fa: %d\n", p->pid, states[p->state], p->name,
                    p->parent->pid);
        else
            cprintf("%d %s %s\n", p->pid, states[p->state], p->name);
    }
    // release(&ptable.lock);
}


/*
 * IDがpidのプロセスにシグナルsigを送信する
 */
static void
send_signal(struct proc *p, int sig)
{
    trace("pid=%d, sig=%d, state=%d, paused=%d", p->pid, sig, p->state, p->paused);
    if (sig == SIGKILL) {
        p->killed = 1;
    } else {
        if (!sigismember(&p->signal.pending, sig))
            sigaddset(&p->signal.pending, sig);
        else
            trace("sig already pending");
    }

    if (p->state == SLEEPING) {
        if (p->paused == 1 && (sig == SIGTERM || sig == SIGINT || sig == SIGKILL)) {
            // For process which are SLEEPING by pause()
            p->paused = 0;
            handle_signal(p, SIGCONT);
        } else if (p->paused == 0 && p->killed != 1) {
            // For stopped process
            handle_signal(p, sig);
        }
    }
}

/*
 * sys_killの実装
 */
long
kill(pid_t pid, int sig)
{
    struct proc *current = thisproc();
    struct proc *p;
    long error = -ESRCH;

    if (pid == 0 || pid < -1) {
        pid_t pgid = pid == 0 ? current->pgid : -pid;
        if (pgid > 0) {
            error = -ESRCH;
            acquire(&ptable.lock);
            for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
                if (p->pgid == pgid) {
                    send_signal(p, sig);
                    error = 0;
                }
            }
            release(&ptable.lock);
        }
        return error;
    } else if (pid == -1) {
        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->pid > 1 && p != current) {
                send_signal(p, sig);
                error = 0;
            }
        }
        release(&ptable.lock);
        return error;
    } else {
        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->pid == pid) {
                send_signal(p, sig);
                error = 0;
            }
        }
        release(&ptable.lock);
        return error;
    }
    return -EINVAL;
}

/*
 * sys_rt_sigsupnedの実装
 */
long
sigsuspend(sigset_t *mask)
{
    struct proc *p = thisproc();
    sigset_t oldmask;

    acquire(&q.siglock);
    p->paused = 1;
    sigdelset(mask, SIGKILL);
    sigdelset(mask, SIGSTOP);
    oldmask = p->signal.mask;
    siginitset(&p->signal.mask, mask);
    release(&q.siglock);

    acquire(&q.lock);
    sleep(p, &q.lock);
    release(&q.lock);

    acquire(&q.siglock);
    p->signal.mask = oldmask;
    release(&q.siglock);
    return -EINTR;
}

/*
 * sys_rt_sigactionの実装
 */
long
sigaction(int sig, struct k_sigaction *act,  struct k_sigaction *oldact)
{
    acquire(&q.siglock);
    struct signal *signal = &thisproc()->signal;
    if (oldact) {
        struct sigaction *action = &signal->actions[sig];
        oldact->handler = action->sa_handler;
        oldact->flags = (unsigned long)action->sa_flags;
        oldact->restorer = action->sa_restorer;
        memmove((void *)&oldact->mask, &action->sa_mask, 8);
        trace("oldact=0x%llx", oldact->handler);
    }
    if (act) {
        struct sigaction *action = &signal->actions[sig];
        action->sa_handler = act->handler;
        action->sa_flags = (int)act->flags;
        action->sa_restorer = act->restorer;
        memmove((void *)&action->sa_mask, &act->mask, 8);
        signal->mask = action->sa_mask;
        sigdelset(&signal->mask, SIGKILL);
        sigdelset(&signal->mask, SIGSTOP);
        trace("sig=%d handler: act=0x%llx, p=0x%llx", sig, act->handler, action->sa_handler);
    }
    release(&q.siglock);

    return 0;
}

/*
 * sys_rt_sigpendingの実装
 */
long
sigpending(sigset_t *pending)
{
    struct proc *p = thisproc();

    acquire(&q.siglock);
    *pending = p->signal.pending;
    release(&q.siglock);
    return 0;
}

/*
 * sys_rt_sigprocmaskの実装
 */
long
sigprocmask(int how, sigset_t *set, sigset_t *oldset, size_t size)
{
    int ret = 0;

    acquire(&q.siglock);
    struct signal *signal = &thisproc()->signal;
    trace("how=%d oldmask=0x%llx set=0x%llx, size=%lld", how, oldmask, set, size);
    if (oldset)
        *oldset = signal->mask;
    if (set) {
        switch(how) {
            case SIG_BLOCK:
                sigorset(&signal->mask, &signal->mask, set);
                break;
            case SIG_UNBLOCK:
                signotset(set, set);
                sigandset(&signal->mask, &signal->mask, set);
                break;
            case SIG_SETMASK:
                signal->mask = *set;
                break;
            default:
                ret = -EINVAL;
        }
    }
    trace(" newmask=0x%llx", signal->mask);
    release(&q.siglock);
    return ret;
}


/*
 * sys_rt_sigreturnの実装
 *   signal_handler処理後の後始末をする
 */
long
sigreturn(void)
{
    struct proc *p = thisproc();

    memmove((void *)p->tf, (void *)p->oldtf, sizeof(struct trapframe));
    return 0;
}

/*
 * シグナルを処理する
 */
void
handle_signal(struct proc *p, int sig)
{
    trace("[%d]: sig=%d, handler=0x%llx", p->pid, sig, p->signal.actions[sig].sa_handler);
    if (!sig) return;
    if (p->signal.actions[sig].sa_handler == SIG_IGN) {
        trace("sig %d handler is SIG_IGN", sig);
    } else if (p->signal.actions[sig].sa_handler == SIG_DFL) {
        switch(sig) {
            case SIGSTOP:
            case SIGTSTP:
            case SIGTTIN:
            case SIGTTOU:
                stop_handler(p);
                break;
            case SIGCONT:
                cont_handler(p);
                break;
            case SIGABRT:
            case SIGBUS:
            case SIGFPE:
            case SIGILL:
            case SIGQUIT:
            case SIGSEGV:
            case SIGSYS:
            case SIGTRAP:
            case SIGXCPU:
            case SIGXFSZ:
                // Core: through
            case SIGALRM:
            case SIGHUP:
            case SIGINT:
            case SIGIO:
            case SIGKILL:
            case SIGPIPE:
            case SIGPROF:
            case SIGPWR:
            case SIGTERM:
            case SIGSTKFLT:
            case SIGUSR1:
            case SIGUSR2:
            case SIGVTALRM:
                term_handler(p);
                break;
            case SIGCHLD:
            case SIGURG:
            case SIGWINCH:
                // Doubt - ignore handler()
                break;
            default:
                break;
        }
    } else {
        trace("call user_handler: sig = %d", sig);
        user_handler(p, sig);
    }

    // clear the pending signal flag
    acquire(&q.siglock);
    sigdelset(&p->signal.pending, sig);
    release(&q.siglock);
}

/*
 * trap処理終了後、ユーザモードに戻る前に実行される
 */
void
check_pending_signal(void)
{
    struct proc *p = thisproc();

    for (int sig = 0; sig < NSIG; sig++) {
        if (sigismember(&p->signal.pending, sig) == 1) {
            trace("pid=%d, sig=%d", p->pid, sig);
            handle_signal(p, sig);
            break;
        }
    }
}

/*
 * 親から引き継いだsignalを調整する
 */
void
flush_signal_handlers(struct proc *p)
{
    struct sigaction *ka;

    for (int i = 0; i < NSIG; i++) {
        ka = &p->signal.actions[i];
        if (ka->sa_handler != SIG_IGN)
            ka->sa_handler = SIG_DFL;
        ka->sa_flags = 0;
        sigemptyset(&ka->sa_mask);
    }
    p->paused = 0;
}

// シグナルハンドラ

/*
 * プロセスを停止する
 */
void
term_handler(struct proc *p)
{
    acquire(&ptable.lock);
    p->killed = 1;
    if (p->state == SLEEPING)
        p->state = RUNNABLE;
    release(&ptable.lock);
}

/*
 * プロセスを継続する
 */
void
cont_handler(struct proc *p)
{
    trace("pid=%d", p->pid);
    wakeup1(p);
}

/*
 * プロセスを停止する
 */
void
stop_handler(struct proc *p)
{
    trace("pid=%d", p->pid);
    acquire(&q.lock);
    sleep(p, &q.lock);
    release(&q.lock);
}

/*
 * ユーザハンドラを処理する
 */
void
user_handler(struct proc *p, int sig)
{
    trace("sig=%d", sig);
    uint64_t sp = p->tf->sp;

    // save the current trapframe from kernel stack to user stack
    sp -= sizeof(struct trapframe);
    memmove((void *)sp, (void *)p->tf, sizeof(struct trapframe));
    p->oldtf = (struct trapframe *)sp;

    // Push the sigret_syscall.S code onto the user stack
    void *sig_ret_code_addr = (void *)execute_sigret_syscall_start;
    uint64_t sig_ret_code_size = (uint64_t)&execute_sigret_syscall_end - (uint64_t)&execute_sigret_syscall_start;

    // return addr for handler
    sp -= sig_ret_code_size;
    uint64_t handler_ret_addr = sp;
    memmove((void *)sp, sig_ret_code_addr, sig_ret_code_size);

    // Push the signal number (これはx86の仕様)
    //sp -= sizeof(uint64_t);
    //*((uint64_t *)sp) = sig;

    // aarc64はレジスタ渡し
    p->tf->x[0] = sig;

    // Push the return address of sigret function
    sp -= sizeof(uint64_t);
    memmove((void *)sp, (void *)&handler_ret_addr, sizeof(uint64_t));

    // change the sp stored in tf
    p->tf->sp = sp;

    // now change the eip to point to the user handler
    p->tf->elr = (uint64_t)p->signal.actions[sig].sa_handler;
}

long
ppoll(struct pollfd *fds, nfds_t nfds) {
    struct proc *p = thisproc();

    if (fds == NULL) {
        p->paused = 1;
        acquire(&q.lock);
        sleep(p, &q.lock);
        release(&q.lock);
        return -EINTR;
    }

    for (int i = 0; i < nfds; i++) {
        fds[i].revents = fds[i].fd == 0 ? POLLIN : POLLOUT;
    }

    return 0;
}

long
setpgid(pid_t pid, pid_t pgid)
{
    struct proc *current = thisproc(), *p, *pp;
    long error = -EINVAL;

    if (!pid) pid = current->pid;
    if (!pgid) pgid = pid;
    if (pgid < 0) return -EINVAL;

    if (pid != current->pid) {
        acquire(&ptable.lock);
        for (pp = ptable.proc; pp < &ptable.proc[NPROC]; pp++) {
            if (pp->pid == pid) {
                p = pp;
                break;
            }
        }
        release(&ptable.lock);
        error = -ESRCH;
        if (!p) goto out;
    } else {
        p = current;
    }

    error = -EINVAL;
    if (p->parent == current) {
        error = -EPERM;
        if (p->sid != current->sid) goto out;
    } else {
        error = -ESRCH;
        if (p != current) goto out;
    }

    if (pgid != pid) {
        acquire(&ptable.lock);
        for (pp = ptable.proc; pp < &ptable.proc[NPROC]; pp++) {
            if (pp->sid == current->sid) {
                release(&ptable.lock);
                goto ok_pgid;
            }
        }
        release(&ptable.lock);
        goto out;
    }

ok_pgid:
    if (current->pgid != pgid) {
        current->pgid = pgid;
    }
    error = 0;
out:
    return error;
}

pid_t
getpgid(pid_t pid)
{
    struct proc *p;

    if (!pid) {
        return thisproc()->pgid;
    } else {
        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->pid == pid) {
                release(&ptable.lock);
                return p->pgid;
            }
        }
        release(&ptable.lock);
        return -ESRCH;
    }
}

uint16_t
get_procs(void)
{
    struct proc *p;
    uint16_t nump = 0;

    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state != UNUSED)
            nump++;
    }
    release(&ptable.lock);
    return nump;
}
