#include <syscall.h>

.global execute_sigret_syscall_start
.global execute_sigret_syscall_end

execute_sigret_syscall_start:
    mov x8, #SYS_rt_sigreturn
    svc #0

execute_sigret_syscall_end:
