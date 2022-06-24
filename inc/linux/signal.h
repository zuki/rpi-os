#ifndef INC_LINUX_SIGNAL_H
#define INC_LINUX_SIGNAL_H

#define NSIG   32

typedef void (*sighandler_t)(int);

// Fake functions
#define SIG_ERR     ((sighandler_t) -1)     // error return
#define SIG_DFL     ((sighandler_t) 0)      // default signal handler
#define SIG_IGN     ((sighandler_t) 1)      // ignore signal

#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGIGT      6
#define SIGBUS      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1     10
#define SIGSEGV     11
#define SIGUSR2     12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGSTKFLT   16
#define SIGCHLD     17
#define SIGCONT     18
#define SIGSTOP     19
#define SIGTSTP     20
#define SIGTTIN     21
#define SIGTTOU     22
#define SIGURG      23
#define SIGXCPU     24
#define SIGXFSZ     25
#define SIGVTALRM   26
#define SIGPROF     27
#define SIGWINCH    28
#define SIGIO       29
#define SIGPOLL     SIGIO
//#define SIGLOST     29
#define SIGPWR      30
#define SIGSYS      31
#define SIGUNUSED   31

#define SIG_BLOCK     0
#define SIG_UNBLOCK   1
#define SIG_SETMASK   2

#define SI_ASYNCNL (-60)
#define SI_TKILL (-6)
#define SI_SIGIO (-5)
#define SI_ASYNCIO (-4)
#define SI_MESGQ (-3)
#define SI_TIMER (-2)
#define SI_QUEUE (-1)
#define SI_USER 0
#define SI_KERNEL 128

#define SA_NOCLDSTOP  1
#define SA_NOCLDWAIT  2
#define SA_SIGINFO    4
#define SA_ONSTACK    0x08000000
#define SA_RESTART    0x10000000
#define SA_NODEFER    0x40000000
#define SA_RESETHAND  0x80000000
#define SA_RESTORER   0x04000000

typedef unsigned long   sigset_t;

struct sigaction {
    sighandler_t    sa_handler;
    sigset_t        sa_mask;
    int             sa_flags;
    void (*sa_restorer)(void);
};

struct k_sigaction {
    sighandler_t    handler;
    unsigned long   flags;
    void (*restorer)(void);
    unsigned long mask;
};

int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);
int sigismember(const sigset_t *set, int signum);
int sigisemptyset(const sigset_t *set);
int signotset(sigset_t *dest, const sigset_t *src);
int sigorset(sigset_t *dest, const sigset_t *left, const sigset_t *right);
int sigandset(sigset_t *dest, const sigset_t *left, const sigset_t *right);
int siginitset(sigset_t *dest, sigset_t *src);

#endif
