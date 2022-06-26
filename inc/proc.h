#ifndef INC_PROC_H
#define INC_PROC_H

#include "types.h"
#include "arm.h"
#include "mmu.h"
#include "trap.h"
#include "spinlock.h"
#include "list.h"
#include "linux/signal.h"
#include "linux/ppoll.h"
#include "linux/capability.h"
#include "linux/resources.h"

#define NPROC           128     // 最大プロセス数
#define NCPU            4       // コア数
#define NOFILE          16      // プロセスがオープンできるファイル数
#define NGROUPS         32      // ユーザが所属できる最大グループ数

/* Stack must always be 16 bytes aligned. */
struct context {
    uint64_t lr0, lr, fp;
    uint64_t x[10];             /* X28 ... X19 */
    uint64_t padding;
    // uint64_t q0[2];             /* V0 */
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

struct signal {
    sigset_t mask;
    sigset_t pending;
    struct sigaction actions[NSIG];
};

/* Per-process state */
struct proc {
    /*
     * Memory layout
     *
     * +----------+
     * |  Kernel  |
     * +----------+  KERNBASE
     * |  Stack   |
     * +----------+  KERNBASE - stksz
     * |   ....   |
     * |   ....   |
     * +----------+  base + sz
     * |   Heap   |
     * +----------+
     * |   Code   |
     * +----------+  base
     * | Reserved |
     * +----------+  0
     *
     */
    size_t base, sz;
    size_t stksz;

    void *pgdir;                /* User space page table. */
    void *kstack;               /* Bottom of kernel stack for this process. */
    enum procstate state;       /* Process state. */

    pid_t pid;                  /* Process ID. */
    pid_t pgid;                 // プロセスグループID
    pid_t sid;                  // セッションID
    struct proc *parent;        /* Parent process */
    struct list_head child;     /* Child list of this process. */
    struct list_head clink;     /* Child list of this process. */

    uid_t uid, euid, suid, fsuid;      /* User, Effective-User, Set-User ID       */
    gid_t gid, egid, sgid, fsgid;      /* Group ID                                */
    gid_t groups[NGROUPS];      // 所属グループ
    int   ngroups;              // 実際に所属しているグループ数
    kernel_cap_t   cap_effective, cap_inheritable, cap_permitted;   // capabilities
    mode_t umask;               // umask

    struct trapframe *tf;       /* Trap frame for current syscall. */
    struct context *context;    /* swtch() here to run process. */
    struct list_head link;      /* linked list of running process. */
    void *chan;                 /* If non-zero, sleeping on chan */
    int killed;                 // If non-zero, have been killed
    int xstate;                 // waitで待っていてる親に返すexit status
    int fdflag;                 // file descriptor flags: 1 bit/file
    struct file *ofile[NOFILE]; // Open files
    struct inode *cwd;          // Current directory
    char name[16];              // Process name (debugging)

    struct signal signal;       // Signal
    struct trapframe *oldtf;    // To save the old trapframe
    int paused;                 //
};

/* Per-CPU state */
struct cpu {
    struct context *scheduler;  /* swtch() here to enter scheduler */
    struct proc *proc;          /* The process running on this cpu or null. */
    struct proc *idle;          /* The idle process. */
    volatile int started;       /* Has the CPU started? */
    struct spinlock lock;
};

extern struct cpu cpu[NCPU];

static inline struct cpu *
thiscpu()
{
    return &cpu[cpuid()];
}

static inline struct proc *
thisproc()
{
    return thiscpu()->proc;
}

void proc_init();
void user_init();
void scheduler();
void sleep(void *chan, struct spinlock *lk);
void wakeup(void *chan);
void yield();
void exit(int);
int  wait4(pid_t pid, int *status, int options, struct rusage *ru);
int  fork();
void procdump();

long kill(pid_t pid, int sig);
long sigsuspend(sigset_t *mask);
long sigaction(int sig, struct k_sigaction *act, struct k_sigaction *oldact);
long sigpending(sigset_t *pending);
long sigprocmask(int how, sigset_t *set, sigset_t *oldset, size_t size);
long sigreturn(void);
void check_pending_signal(void);
void stop_handler(struct proc *p);
void cont_handler(struct proc *p);
void term_handler(struct proc *p);
void handle_signal(struct proc *p , int sig);
void user_handler(struct proc *p, int sig);
void flush_signal_handlers(struct proc *p);
long ppoll(struct pollfd *fds, nfds_t nfds);
long setpgid(pid_t, pid_t);
pid_t getpgid(pid_t);
uint16_t get_procs();

// sigret_syscall.S
void execute_sigret_syscall_start(void);
void execute_sigret_syscall_end(void);

static inline int capable(int cap)
{
   if (cap_raised(thisproc()->cap_effective, cap))
      return 1;
   return 0;
}

#endif
