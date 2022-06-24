
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "fs.h"

char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;
    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--) ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    //memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

char *
fmttime(time_t time)
{
    static char mtime_s[12];

    struct tm *tm = localtime(&time);
    sprintf(mtime_s, "%2d %2d %02d:%02d",
        tm->tm_mon + 1, tm->tm_mday,  tm->tm_hour, tm->tm_min);
    return mtime_s;
}

void
ls(char *path)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(stderr, "ls: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (S_ISREG(st.st_mode)) {
        printf("%04o %4ld %5ld %s %s\n", st.st_mode, st.st_ino,
               st.st_size, fmttime(st.st_mtime), fmtname(path));
    } else if (S_ISDIR(st.st_mode)) {
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
            fprintf(stderr, "ls: path too long\n");
        } else {
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if (stat(buf, &st) < 0) {
                    fprintf(stderr, "ls: cannot stat %s\n", buf);
                    continue;
                }
                printf("%04o %4ld %5ld %s %s\n", st.st_mode,
                       st.st_ino, st.st_size, fmttime(st.st_mtime), fmtname(buf));
            }
        }
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc < 2)
        ls(".");
    else
        for (int i = 1; i < argc; i++)
            ls(argv[i]);
    return 0;
}
