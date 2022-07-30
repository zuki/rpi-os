#ifndef INC_SYSCALL1_H
#define INC_SYSCALL1_H

#include "types.h"
#include "trap.h"
#include "linux/syscall.h"
#include "linux/fcntl.h"

#define MAXARG      32
#define MAXENV      64

// kern/syscall.c
int in_user(void *s, size_t n);
long argstr(int, char **);
long argint(int, int *);
long argu64(int n, uint64_t * ip);
long argptr(int, char **, size_t);
long fetchstr(uint64_t, char **);
long sys_clock_gettime();
long sys_sched_getaffinity();
long sys_prlimit64();
long sys_sysinfo();
long sys_nanosleep();
long sys_getrandom();
long sys_uname();
long sys_clock_settime();
long sys_ppoll();
long sys_getitimer();
long sys_setitimer();
long sys_madvise();
long syscall1(struct trapframe *);

// kern/sysproc.c
size_t sys_brk();
long sys_yield();
long sys_clone();
long sys_wait4();
long sys_exit();
long sys_exit_group();
long sys_rt_sigprocmask();
long sys_rt_sigsuspend();
long sys_rt_sigaction();
long sys_rt_sigpending();
long sys_rt_sigreturn();
long sys_kill();
long sys_tkill();
mode_t sys_umask();
long sys_getpgid();
long sys_setpgid();
long sys_setregid();
long sys_setgid();
long sys_setreuid();
long sys_setuid();
long sys_setresuid();
long sys_setresgid();
long sys_getresuid();
long sys_getresgid();
long sys_getuid();
long sys_geteuid();
long sys_getgid();
long sys_getegid();
long sys_setfsuid();
long sys_setfsgid();
long sys_getgroups();
long sys_setgroups();
long sys_gettid();
long sys_getpid();
long sys_getppid();
long sys_set_tid_address();
long sys_mmap();
long sys_munmap();
long sys_msync();
void *sys_mremap();
long sys_mprotect();
long sys_madvise();

// kern/sysfile.c
long sys_execve();
long sys_dup();
long sys_dup3();
long sys_fcntl();
long sys_ioctl();
long sys_pipe2();
long sys_fstat();
long sys_fstatat();
ssize_t sys_read();
ssize_t sys_readv();
ssize_t sys_write();
ssize_t sys_writev();
ssize_t sys_lseek();
long sys_fsync();
long sys_fdatasync();
long sys_close();
long sys_openat();
long sys_getdents64();
long sys_mkdirat();
long sys_mknodat();
long sys_unlinkat();
long sys_symlinkat();
ssize_t sys_readlinkat();
long sys_linkat();
long sys_chdir();
long sys_faccessat();
long sys_faccessat2();
long sys_fchmodat();
long sys_fchownat();
long sys_fchown();
long sys_umount2();
long sys_mount();
long sys_renameat();
long sys_renameat2();
void *sys_getcwd();
long sys_fadvise64();
long sys_utimensat();
long sys_pread64();
//int dirunlink(struct inode *, char *, uint32_t);
//int direntlookup(struct inode *, int, struct dirent *);

// kern/exec.c
//long do_execve(char *, int, int, char **, char **);

#endif
