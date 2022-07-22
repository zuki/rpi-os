#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

char *argv[] = { "dash", "/etc/inittab", 0 };
//char *envp[] = { "TEST_ENV=FROM_INIT", "TZ=JST-9", 0 };
char *envp[] = { "TZ=JST-9", 0 };

int
main()
{
    int pid, wpid;
    int status;
    static init = 1;

    if (open("/dev/tty", O_RDWR) < 0) {
        mknod("/dev/tty", (S_IFCHR | 0777), makedev(1, 0));
        open("/dev/tty", O_RDWR);
    }
    dup(0);                     // stdout
    dup(0);                     // stderr

    while (1) {
        //printf("init: starting sh\n");
        pid = fork();
        if (pid < 0) {
            printf("init: fork failed\n");
            exit(1);
        }
        if (pid == 0) {
            //execve("/bin/sh", argv, envp);
            if (init)
                execve("/usr/bin/dash", argv, envp);
            else
                execve("/bin/getty", 0, 0);
            printf("init: exec sh failed\n");
            exit(1);
        }
        while ((wpid = wait(&status)) >= 0 && wpid != pid)
            printf("zombie!\n");
        init = 0;
        //printf("[%d] pid=%d, wpid=%d, status=%d\n", getpid(), pid, wpid, status);
    }

    return 0;
}
