//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "linux/errno.h"

#include "syscall1.h"
#include "types.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "string.h"
#include "console.h"
#include "log.h"
#include "fs.h"
#include "file.h"
#include "pipe.h"
#include "linux/fcntl.h"
#include "linux/ioctl.h"
#include "linux/termios.h"

extern int execve(const char *, char *const, char *const);

struct iovec {
    void *iov_base;             /* Starting address. */
    size_t iov_len;             /* Number of bytes to transfer. */
};

/*
 * Fetch the nth word-sized system call argument as a file descriptor
 * and return both the descriptor and the corresponding struct file.
 */
static int
argfd(int n, int *pfd, struct file **pf)
{
    int fd;
    struct file *f;

    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = thisproc()->ofile[fd]) == 0)
        return -1;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

static long
dupfd(int fd, int from)
{
    struct file *f = thisproc()->ofile[fd];
    if (!f) return -EBADF;
    filedup(f);
    return fdalloc(f, from);
}

long
sys_dup()
{
    int fd;

    if (argint(0, &fd) < 0)
        return -EINVAL;
    return dupfd(fd, 0);
}

long
sys_dup3()
{
    struct file *f;
    int fd1, fd2, flags;
    struct proc *p = thisproc();

     if (argint(0, &fd1) < 0 || argint(1, &fd2) < 0 || argint(2, &flags) < 0)
        return -EINVAL;

    trace("fd1=%d, fd2=%d, flags=%x", fd1, fd2, flags);

    if (flags & ~O_CLOEXEC) return -EINVAL;
    if (fd1 == fd2) return -EINVAL;
    if (fd1 < 0 || fd1 >= NOFILE) return -EBADF;
    if (fd2 < 0 || fd2 >= NOFILE) return -EBADF;

    f = p->ofile[fd1];
    if (p->ofile[fd2])
        fileclose(p->ofile[fd2]);
    p->ofile[fd2] = filedup(f);
    if (flags & O_CLOEXEC)
        bit_add(p->fdflag, fd2);

    return fd2;
}


long
sys_pipe2()
{
    int pipefd[2], flags;
    struct file *rf, *wf;
    struct proc *p = thisproc();
    int fd0, fd1;

    if (argptr(0, (char **)&pipefd, sizeof(int)*2) < 0 || argint(1, &flags) < 0)
        return -EINVAL;

    debug("pipefd: 0x%p, flags=0x%x\n", pipefd, flags);

    if (flags & ~PIPE2_FLAGS) {
        warn("invalid flags=%d", flags);
        return -EINVAL;
    }

    if (pipealloc(&rf, &wf, flags) < 0) {
        warn("pipealloc failed");
        return -ENFILE;
    }

    fd0 = -1;
    if ((fd0 = fdalloc(rf, 0)) < 0 || (fd1 = fdalloc(wf, 0)) < 0) {
        if (fd0 >= 0)
            p->ofile[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        warn("fdalloc failed");
        return -EMFILE;
    }

    //memmove((void *)pipefd, &fd0, sizeof(int));
    //memmove((void *)pipefd+sizeof(int), &fd1, sizeof(int));
    pipefd[0] = fd0;
    pipefd[1] = fd1;

    if (flags & O_CLOEXEC) {
        bit_add(p->fdflag, fd0);
        bit_add(p->fdflag, fd1);
    }

    debug("fd0=%d, fd1=%d, pipefd[%d, %d]", fd0, fd1, pipefd[0], pipefd[1]);
    return 0;
}

long
sys_fcntl()
{
    struct file *f;
    struct proc *p = thisproc();
    int fd, cmd, args;

    if (argfd(0, &fd, &f) < 0 || argint(1, &cmd) < 0 || argint(2, &args))
        return -EINVAL;
    debug("fd=%d, cmd=0x%x, args=%d", fd, cmd, args);

    switch (cmd) {
        case F_DUPFD:
            return dupfd(fd, args);

        case F_GETFD:
            return bit_test(p->fdflag, fd) ? FD_CLOEXEC : 0;

        case F_SETFD:
            if (args & FD_CLOEXEC)
                bit_add(p->fdflag, fd);
            else
                bit_remove(p->fdflag, fd);
            return 0;

        case F_GETFL:
            return (f->flags & (FILE_STATUS_FLAGS | O_ACCMODE));

        case F_SETFL:
            f->flags = ((args & FILE_STATUS_FLAGS) | (f->flags & O_ACCMODE));
            return 0;
    }

    return -EINVAL;
}

long
sys_ioctl()
{
    int fd;
    uint64_t req;
    struct termios *term;
    struct winsize *win;
    struct file *f;
    pid_t *pgid_p;

    if (argfd(0, &fd, &f) < 0 || argu64(1, &req) < 0)
        return -EINVAL;

    debug("fd: %d, req: 0x%llx, f->type: %d", fd, req, f->ip->type);

    if (f->ip->type != T_DEV) return -ENOTTY;

    trace("fd=%d, req=0x%llx\n", fd, req);

    switch (req) {
        case TCGETS:
            if (argptr(2, (char **)&term, sizeof(struct termios)) < 0)
                return -EINVAL;
            if (term == NULL) return -EINVAL;
            memmove(term, devsw[f->ip->major].termios, sizeof(struct termios));
            break;
        case TCSETS:
        case TCSETSW:
        case TCSETSF:
            if (argptr(2, (char **)&term, sizeof(struct termios)) < 0)
                return -EINVAL;
            if (term == NULL) return -EINVAL;
            memmove(devsw[f->ip->major].termios, term, sizeof(struct termios));
            break;
        case TCSBRK:
        case TCFLSH:            // TODO: cosole.flushを実装すること
            break;
        case TIOCGWINSZ:
            if (argptr(2, (char **)&win, sizeof(struct winsize)) < 0)
                return -EINVAL;
            if (win == NULL) return -EINVAL;
            win->ws_row = 24;
            win->ws_col = 80;
            break;
        case TIOCSWINSZ:
            // Windowサイズ設定: 当面何もしない
            break;
        case TIOCSPGRP:  // TODO: 本来、dev(tty)用なのでp->sgid? ¥
            if (argptr(2, (char **)&pgid_p, sizeof(pid_t)) < 0)
                return -EINVAL;
            thisproc()->pgid = *pgid_p;
            break;
        case TIOCGPGRP:
            if (argptr(2, (char **)&pgid_p, sizeof(pid_t)) < 0)
                return -EINVAL;
            if (pgid_p == NULL) return -EINVAL;
            *pgid_p = thisproc()->pgid;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

ssize_t
sys_read()
{
    struct file *f;
    size_t n;
    char *p;

    if (argfd(0, 0, &f) < 0 || argu64(2, (uint64_t *)&n) < 0 || argptr(1, &p, n) < 0)
        return -EINVAL;

    return fileread(f, p, n);
}

ssize_t
sys_readv()
{
    int fd, iovcnt;
    struct iovec *iov, *ip;
    struct file *f;

    if (argfd(0, &fd, &f) < 0 ||
        argint(2, &iovcnt) < 0 ||
        argptr(1, (char **)&iov, iovcnt * sizeof(struct iovec)) < 0) {
        return -EINVAL;
    }

    if (fd < 0 || fd >= NOFILE) return -EBADF;
    if (iovcnt <= 0 || iovcnt > UIO_MAXIOV) return -EINVAL;

    trace("fd: %d, iovcnt=%d", fd, iovcnt);

    ssize_t tot = 0;
    for (ip = iov; ip < iov + iovcnt; ip++) {
        if (ip->iov_len < 0) return -EINVAL;
        if (!in_user(iov->iov_base, iov->iov_len)) return -EFAULT;
        tot += fileread(f, ip->iov_base, ip->iov_len);
    }
    return tot;
}

ssize_t
sys_write()
{
    struct file *f;
    ssize_t n;
    char *p;
    int fd;

    if (argfd(0, &fd, &f) < 0 || argu64(2, (uint64_t *)&n) < 0 || argptr(1, &p, n) < 0)
        return -EINVAL;

    debug("fd: %d, p: %s, n: %lld, f->type: %d", fd, p, n, f->type);
    return filewrite(f, p, n);
}


ssize_t
sys_writev()
{
    struct file *f;
    int fd, iovcnt;
    struct iovec *iov, *p;
    if (argfd(0, &fd, &f) < 0 ||
        argint(2, &iovcnt) < 0 ||
        argptr(1, (char **)&iov, iovcnt * sizeof(struct iovec)) < 0) {
        return -1;
    }
    trace("fd %d, iovcnt: %d", fd, iovcnt);

    size_t tot = 0;
    for (p = iov; p < iov + iovcnt; p++) {
        if (!in_user(p->iov_base, p->iov_len)) return -EFAULT;
        tot += filewrite(f, p->iov_base, p->iov_len);
    }
    return tot;
}

ssize_t
sys_lseek()
{
    off_t offset;
    int whence;
    struct file *f;

    if (argfd(0, 0, &f) < 0 || argu64(1, (uint64_t *)&offset) < 0
     || argint(2, &whence) < 0)
        return -EINVAL;

    if (whence & ~(SEEK_SET|SEEK_CUR|SEEK_END)) return -EINVAL;

    return filelseek(f, offset, whence);
}

long
sys_close()
{
    int fd;
    struct file *f;

    if (argfd(0, &fd, &f) < 0)
        return -1;
    thisproc()->ofile[fd] = 0;
    fileclose(f);
    bit_remove(thisproc()->fdflag, fd);
    return 0;
}

long
sys_fstat()
{
    int fd;
    struct file *f;
    struct stat *st;

    if (argfd(0, &fd, &f) < 0 || argptr(1, (void *)&st, sizeof(*st)) < 0)
        return -EINVAL;
    if (!in_user(st, sizeof(st))) return -EFAULT;

    trace("fd %d", fd);
    return filestat(f, st);
}

long
sys_fstatat()
{
    int dirfd, flags;
    char *path;
    struct stat *st;

    if (argint(0, &dirfd) < 0 ||
        argstr(1, &path) < 0 ||
        argptr(2, (void *)&st, sizeof(*st)) < 0 || argint(3, &flags) < 0)
        return -EINVAL;

    if (dirfd != AT_FDCWD) {
        warn("dirfd unimplemented");
        return -EINVAL;
    }
    // TODO: AT_EMPTY_PATH, AT_SYMLINK_NOFOLLOWは実装する
    /* 当面、無視する: AT_EMPTY_PATH の使用があった
    if (flags != 0) {
        warn("flags unimplemented: flags=%d", flags);
        return -EINVAL;
    }
    */

    struct inode *ip;
    if ((ip = namei(path)) == 0) {
        return -ENOENT;
    }
    ilock(ip);
    stati(ip, st);
    iunlockput(ip);

    return 0;
}

/* Create the path new as a link to the same inode as old. */
long
sys_linkat()
{
    char *newpath, *oldpath;
    int oldfd, newfd, flags;

    if (argint(0, &oldfd) < 0 || argstr(1, &oldpath) < 0
     || argint(2, &newfd) < 0 || argstr(3, &newpath) < 0 || argint(4, &flags))
        return -EINVAL;

    trace("oldfd: %d, oldpath: %s, newfd: %d, newpath: %s, flags: %d", oldfd, oldpath, newfd, newpath, flags);

    // TODO: AT_FDCWD以外の実装
    if (oldfd != AT_FDCWD || newfd != AT_FDCWD) return -EINVAL;
    // TODO: AT_SYMLINK_FOLLOW は実装する
    if (flags) return -EINVAL;

    return filelink(oldpath, newpath);
}


long
sys_symlinkat()
{
    char *target, *linkpath;
    int fd;

    if (argstr(0, &target) < 0 || argint(1, &fd) < 0 || argstr(2, &linkpath) < 0)
        return -EINVAL;

    trace("fd: %d, target: %s, path: %s", fd, target, linkpath);

    // TODO: AT_FDCWD以外の実装
    if (fd != AT_FDCWD) return -EINVAL;

    if (strncmp(target, "", 1) == 0 || strncmp(linkpath, "", 1) == 0)
        return -ENOENT;

    return filesymlink(target, linkpath);
}

long
sys_unlinkat()
{
    char *path;
    int dirfd, flags;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
     || argint(2, &flags) < 0)
        return -EINVAL;

    if (dirfd != AT_FDCWD) return -EINVAL;

    if (flags & ~AT_REMOVEDIR) return -EINVAL;

    return fileunlink(path, flags);
}

ssize_t
sys_readlinkat()
{
    char *path, *buf;
    int dirfd;
    size_t bufsiz;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
     || argu64(3, &bufsiz) < 0 || argptr(2, (char **)&buf, bufsiz) < 0)
        return -EINVAL;
    if ((ssize_t)bufsiz <= 0) return -EINVAL;
    if (!in_user(buf, bufsiz))
        return -EFAULT;

    // TODO: AT_FDCWD以外の実装
    if (dirfd != AT_FDCWD) return -EINVAL;

    trace("dirfd=0x%x, path=%s, buf=%p, bufsiz=%lld", dirfd, path, buf, bufsiz);

    if (!strncmp(path, "/proc/self/fd/", 14)) {   // TODO: ttyをちゃんと実装
        int fd = path[14] - '0';                  // /proc/self/fd/n -> /dev/tty/n, /dev/pts/n
        if (fd < 3) {
            memmove(buf, "/dev/tty", strlen("/dev/tty"));
            buf[8] = 0;
            return 9;
        }
    }

    return filereadlink(path, buf, bufsiz);
}

long
sys_openat()
{
    char *path;
    int dirfd, flags;
    mode_t mode;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
        || argint(2, &flags) < 0 || argint(3, (int *)&mode) < 0)
        return -EINVAL;

    trace("dirfd %d, path '%s', flag 0x%x, mode0x%x", dirfd, path, flags, mode);

    if (dirfd != AT_FDCWD) {
        warn("dirfd unimplemented");
        return -EINVAL;
    }
    if ((flags & O_LARGEFILE) == 0) {
        warn("expect O_LARGEFILE in open flags");
        return -EINVAL;
    }

    return fileopen(path, flags, mode);

}

long
sys_mkdirat()
{
    int dirfd;
    char *path;
    mode_t mode;
    struct inode *ip;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
        || argint(2, (int *)&mode) < 0)
        return -EINVAL;

    debug("dirfd %d, path '%s', mode 0x%x", dirfd, path, mode);

    if (dirfd != AT_FDCWD) {
        warn("dirfd unimplemented");
        return -EINVAL;
    }


    begin_op();
    if ((long)(ip = create(path, T_DIR, 0, 0, mode | S_IFDIR)) == 0) {
        end_op();
        return (long)ip;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

long
sys_mknodat()
{
    struct inode *ip;
    char *path;
    int dirfd;
    mode_t mode;
    dev_t  dev;
    uint16_t major = 0, minor = 0, type;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
        || argint(2, (int *)&mode) < 0 || argu64(3, &dev))
        return -EINVAL;

    if (dirfd != AT_FDCWD) {
        warn("dirfd unimplemented");
        return -EINVAL;
    }
    trace("path '%s', mode 0x%x, dev 0x%llx %d:%d", path, mode, minor);

    begin_op();
    if (S_ISDIR(mode))
        type = T_DIR;
    else if (S_ISREG(mode))
        type = T_FILE;
    else if (S_ISCHR(mode) || S_ISBLK(mode))
        type = T_DEV;
    else if (S_ISLNK(mode))
        type = T_SYMLINK;
    else {
        warn("%d is not supported yet", mode & S_IFMT);
        return -EINVAL;
    }
    if (type == T_DEV) {
        major = major(dev);
        minor = minor(dev);
    }

    if ((long)(ip = create(path, type, major, minor, mode)) < 0) {
        end_op();
        return (long)ip;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

long
sys_chdir()
{
    char *path;
    struct inode *ip;
    struct proc *curproc = thisproc();

    if (argstr(0, &path) < 0)
        return -EINVAL;

    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -ENOENT;
    }
    ilock(ip);
    if (ip->type != T_DIR && ip->type != T_MOUNT) {
        iunlockput(ip);
        end_op();
        return -ENOTDIR;
    }
    iunlock(ip);
    iput(curproc->cwd);
    end_op();
    curproc->cwd = ip;
    return 0;
}

long
sys_execve()
{
    char *p;
    void *argv, *envp;
    if (argstr(0, &p) < 0 || argu64(1, (uint64_t *) & argv) < 0
        || argu64(2, (uint64_t *) & envp) < 0)
        return -1;
    return execve(p, argv, envp);
}


long
sys_fadvise64()
{
    int fd ;
    off_t offset;
    off_t len;
    int advice;

    if (argfd(0, &fd, 0) < 0 || argu64(1, (uint64_t *)&offset) < 0
     || argu64(2, (uint64_t *)&len) < 0 || argint(3, &advice) < 0)
        return -EINVAL;

    if (advice < POSIX_FADV_NORMAL && advice > POSIX_FADV_NOREUSE)
        return -EINVAL;

    trace("fd=%d, offset=%d, len=%d, advice=0x%x", fd, offset, len, advice);

    return 0;
}

long
sys_getdents64()
{
    int fd;
    char *dirp;
    size_t count;
    struct file *f;

    if (argfd(0, &fd, &f) < 0 || argu64(2, &count) < 0 || argptr(1, &dirp, count) < 0)
        return -EINVAL;
    if (!in_user(dirp, count))
        return -EFAULT;

    trace("fd: %d, dirp: 0x%p, count: 0x%llx", fd, dirp, count);

/*
    if (strcmp("ext2", f->ip->fs_t->name) == 0)
        ret = ext2_getdents(f, dirp, count);
    else if (strcmp("v6", f->ip->fs_t->name) == 0)
        ret = v6_getdents(f, dirp, count);
    else
        ret = -ENOENT;
*/
    return getdents(f, dirp, count);
}

long
sys_fsync()
{
    int fd;

    if (argfd(0, &fd, 0) < 0)
        return -EINVAL;

    // xv6 file systemは常にflushされているので何もしない
    return 0;
}

long
sys_fdatasync()
{
    int fd;

    if (argfd(0, &fd, 0) < 0)
        return -EINVAL;

    // xv6 file systemは常にflushされているので何もしない
    return 0;
}


long
sys_utimensat()
{
    int dirfd, flags;
    char *path;
    struct timespec times[2];

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
     || argptr(2, (char **)&times, 2 * sizeof(struct timespec)) < 0
     || argint(3, &flags) < 0)
        return -EINVAL;


    debug("dirfd: %d, path: %s, times[0]: (%lld, %lld), times[1]: (%lld, %lld), flags: 0x%x", dirfd, path, times[0].tv_sec, times[0].tv_nsec, times[1].tv_sec, times[1].tv_nsec, flags);

    // pathがNULLの場合は、ファイルを特定できないので処理せず0を返す
    if (path == NULL) return 0;

    // TODO: dirfdの実装
    if (dirfd != AT_FDCWD) return -EINVAL;
    // TODO: AT_SYMLINK_NOFOLLOWを実装
    if (flags) return -EINVAL;

    return utimensat(path, times);
}

long
sys_fchmodat()
{
    int dirfd, flags;
    char *path;
    mode_t mode;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
     || argint(2, (int *)&mode) < 0 || argint(3, &flags) < 0)
        return -EINVAL;

    // TODO: AT_FDCWD以外の実装
    if (dirfd != AT_FDCWD) return -EINVAL;

    return filechmod(path, mode);
}

long
sys_fchown()
{
    int fd;
    uid_t owner;
    gid_t group;
    struct file *f;

    if (argfd(0, &fd, &f) < 0 || argint(1, (int *)&owner) < 0
     || argint(2, (int *)&group) < 0)
        return -EINVAL;

    return filechown(f, 0, owner, group);
}

long
sys_fchownat()
{
    int dirfd, flags;
    char *path;
    uid_t owner;
    gid_t group;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
     || argint(2, (int *)&owner) < 0 || argint(3, (int *)&group) < 0
     || argint(4, &flags) < 0)
        return -EINVAL;

    // TODO: AT_FDCWD以外の実装
    if (dirfd != AT_FDCWD) return -EINVAL;
    // TODO: flagsの実装

    trace("dirfd=%d, path=%s, uid=%d, gid=%d, flags=%d\n", dirfd, path, owner, group, flags);

    return filechown(0, path, owner, group);
}

/* カレントワーキングディレクトリ名の取得 */
void *
sys_getcwd()
{
    char *buf;
    size_t size;
    struct proc *p = thisproc();
    struct inode *cwd, *dp;
    struct dirent de;
    int i, n, pos = 0;

    if (argu64(1, &size) < 0 || argptr(0, &buf, size) < 0)
        return (void *)-EINVAL;

    cwd = idup(p->cwd);
    if (cwd->inum == ROOTINO) goto root;
    while (1) {
        dp = dirlookup(cwd, "..", 0);
        ilock(dp);
        if (direntlookup(dp, cwd->inum, &de) < 0)
            goto bad;
        n = strlen(de.name);
        if ((n + pos + 2) > size)
            goto bad;

        iput(cwd);
        iunlock(dp);
        for (i = 0; i < n; i++) {
            buf[pos + i] = de.name[n - i - 1];  // bufに逆順に詰める
        }
        pos += i;
        if (dp->inum == ROOTINO) break;
        buf[pos++] = '/';
        cwd = idup(dp);
        iput(dp);
    }
    iput(dp);
root:
    buf[pos++] = '/'; buf[pos] = 0;             // NULL終端

   // bufを反転する
    for (i = 0; i > (pos + 1) / 2; i++) {
        char c = buf[i];
        buf[i] = buf[pos - i];
        buf[pos - i] = c;
    }
    trace("buf: %s", buf);
    return buf;

bad:
    iput(cwd);
    iunlockput(dp);
    return NULL;
}

long
sys_faccessat()
{
    int dirfd, mode;
    char *path;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
     || argint(2, &mode) < 0)
        return -EINVAL;

    // TODO: dirfdは未実装
    if (dirfd != AT_FDCWD) return -EINVAL;

    return faccess(path, mode, 0);
}

long
sys_faccessat2()
{
    int dirfd, mode, flags;
    char *path;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0
     || argint(2, &mode) < 0 || argint(3, &flags) < 0)
        return -EINVAL;

    // TODO: dirfdは未実装
    if (dirfd != AT_FDCWD) return -EINVAL;
    // TODO: AT_SYMLINK_NOFOLLOWの実装
    if (flags & AT_SYMLINK_NOFOLLOW) return -EINVAL;

    return faccess(path, mode, flags);
}

long
sys_renameat2()
{
    int olddirfd, newdirfd;
    char *oldpath, *newpath;
    uint32_t flags;

    if (argint(0, &olddirfd) < 0 || argstr(1, &oldpath) < 0
     || argint(2, &newdirfd) < 0 || argstr(3, &newpath) < 0
     || argint(4, (int *)&flags) < 0)
        return -EINVAL;

    // TODO: olddirfd, newdirfd, flagsは未実装
    if (olddirfd != AT_FDCWD || newdirfd != AT_FDCWD) return -EINVAL;
    // TODO: flagsの実装

    if (strcmp(oldpath, newpath) == 0) return 0;

    return filerename(oldpath, newpath);

}
