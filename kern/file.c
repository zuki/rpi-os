/* File descriptors */

#include "linux/errno.h"

#include "types.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "console.h"
#include "log.h"
#include "pipe.h"
#include "clock.h"
#include "string.h"
#include "linux/stat.h"

struct devsw devsw[NDEV];
struct {
    struct spinlock lock;
    struct file file[NFILE];
} ftable;

/* Is the directory dp empty except for "." and ".." ? */
static int
isdirempty(struct inode *dp)
{
    struct dirent de;

    for (ssize_t off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
        if (readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
            panic("readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}

/* Optional since BSS is zero-initialized. */
void
fileinit()
{
    initlock(&ftable.lock);
}

/* Allocate a file structure. */
struct file *
filealloc()
{
    struct file *f;

    acquire(&ftable.lock);
    for (f = ftable.file; f < ftable.file + NFILE; f++) {
        if (f->ref == 0) {
            f->ref = 1;
            release(&ftable.lock);
            return f;
        }
    }
    release(&ftable.lock);
    return 0;
}

/* Increment ref count for file f. */
struct file *
filedup(struct file *f)
{
    acquire(&ftable.lock);
    if (f->ref < 1)
        panic("filedup");
    f->ref++;
    release(&ftable.lock);
    return f;
}

long
fileopen(char *path, int flags, mode_t mode)
{
    struct inode *ip;
    struct file *f;
    char buf[512];
    int fd, n;

    begin_op();
    if (flags & O_CREAT) {
        ip = namei(path);
        if (ip && flags & O_EXCL) {
            iput(ip);
            end_op();
            warn("O_CREAT && O_EXCL and  %s exists", path);
            return -EEXIST;
        } else if (!ip) {
            if ((ip = create(path, T_FILE, 0, 0, mode | S_IFREG)) == 0) {
                end_op();
                warn("cant create %s", path);
                return -EDQUOT;
            }
        } else {
            ilock(ip);
        }
    } else {
loop:
        if ((ip = namei(path)) == 0) {
            end_op();
            warn("cant namei %s", path);
            return -ENOENT;
        }
        ilock(ip);
        if (ip->type == T_DIR && (flags & O_ACCMODE) != 0) {
            iunlockput(ip);
            end_op();
            warn("wrong flags 0x%llx", flags);
            return -EINVAL;
        }
        if (ip->type == T_SYMLINK) {
            if ((n = readi(ip, buf, 0, sizeof(buf) - 1)) <= 0) {
                iunlockput(ip);
                warn("couldn't read sysmlink target");
                return -ENOENT;
            }
            buf[n] = 0;
            path = buf;
            iunlockput(ip);
            goto loop;
        }

    }
    int readable = FILE_READABLE((int)flags);
    int writable = FILE_WRITABLE((int)flags);
/*  TODO: 権限チェック
    if (readable && ((error = ip->iops->permission(ip, MAY_READ)) < 0))
        goto bad;
    if (writable && ((error = ip->iops->permission(ip, MAY_WRITE)) < 0))
        goto bad;
*/
    if ((f = filealloc()) == 0 || (fd = fdalloc(f, 0)) < 0) {
        iunlock(ip);
        end_op();
        if (f) fileclose(f);
        warn("cant alloc file\n");
        return -ENOSPC;
    }
    iunlock(ip);
    end_op();

    f->type     = FD_INODE;
    f->ip       = ip;
    f->off      = (flags & O_APPEND) ? ip->size : 0;
    f->flags    = flags;
    f->readable = readable;
    f->writable = writable;
    if (flags & O_CLOEXEC)
        bit_add(thisproc()->fdflag, fd);
    trace("proc[%d], path: %s, fd: %d, flags: %d", thisproc()->pid, path, fd, ip->flags);
    return fd;

bad:
    iunlockput(ip);
    end_op();
    return -EACCES;
}

/* Close file f. (Decrement ref count, close when reaches 0.) */
void
fileclose(struct file *f)
{
    struct file ff;

    acquire(&ftable.lock);
    if (f->ref < 1)
        panic("fileclose");
    if (--f->ref > 0) {
        release(&ftable.lock);
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&ftable.lock);

    if (ff.type == FD_PIPE) {
        pipeclose(ff.pipe, ff.writable);
    } else if (ff.type == FD_INODE) {
        begin_op();
        iput(ff.ip);
        end_op();
    }
}

/* Get metadata about file f. */
long
filestat(struct file *f, struct stat *st)
{
    if (f->type == FD_INODE) {
        ilock(f->ip);
        stati(f->ip, st);
        iunlock(f->ip);
        return 0;
    } else if (f->type == FD_PIPE) {
        memset(st, 0, sizeof(struct stat));
        st->st_mode = (S_IFCHR|S_IRUSR|S_IWUSR);
        st->st_size = 0;
        st->st_nlink = 1;
        st->st_blocks = 0;
        st->st_blksize = 4096;
        return 0;
    }
    return -EFAULT;
}

/* Read from file f. */
ssize_t
fileread(struct file *f, char *addr, ssize_t n)
{
    ssize_t r;

    if (f->readable == 0) return -EBADF;
    if (f->type == FD_PIPE)
        return piperead(f->pipe, addr, n);
    if (f->type == FD_INODE) {
        begin_op();
        ilock(f->ip);
        if ((r = readi(f->ip, addr, f->off, n)) > 0)
            f->off += r;
        clock_gettime(CLOCK_REALTIME, &f->ip->atime);
        iunlock(f->ip);
        end_op();
        return r;
    }
    panic("fileread");
    return -EINVAL;
}

/* Write to file f. */
ssize_t
filewrite(struct file *f, char *addr, ssize_t n)
{
    ssize_t r;
    struct timespec ts;

    if (f->writable == 0) return -EBADF;
    if (f->type == FD_PIPE)
        return pipewrite(f->pipe, addr, n);
    if (f->type == FD_INODE) {
        /*
         * Write a few blocks at a time to avoid exceeding
         * the maximum log transaction size, including
         * i-node, indirect block, allocation blocks,
         * and 2 blocks of slop for non-aligned writes.
         * This really belongs lower down, since writei()
         * might be writing a device like the console.
         */
        ssize_t max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * 512;
        ssize_t i = 0;
        while (i < n) {
            ssize_t n1 = MIN(max, n - i);
            begin_op();
            ilock(f->ip);
            if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
                f->off += r;
            clock_gettime(CLOCK_REALTIME, &ts);
            f->ip->mtime = f->ip->atime = ts;
            iunlock(f->ip);
            end_op();

            if (r < 0) break;
            if (r != n1)
                panic("short filewrite: r=%lld, n1=%lld", r, n1);
            i += r;
        }
        return i == n ? n : -1;
    }
    panic("filewrite");
    return -EINVAL;
}

ssize_t
filelseek(struct file *f, off_t offset, int whence)
{
    if (!f) return -EBADF;

    switch(whence) {
        case SEEK_SET:
            if (offset < 0)
                goto bad;
            else
                f->off = offset;
            break;
        case SEEK_CUR:
            if (f->off + offset < 0)
                goto bad;
            else
                f->off += offset;
            break;
        case SEEK_END:
            if (f->ip->size + offset < 0)
                goto bad;
            else
                f->off = f->ip->size + offset;
            break;
        default:
            goto bad;
    }
    return f->off;

bad:
    return -EINVAL;
}

long
filelink(char *old, char *new)
{
    char name[DIRSIZ];
    struct inode *dp, *ip;
    long error;

    begin_op();
    if ((ip = namei(old)) == 0) {
        end_op();
        return -ENOENT;
    }

    ilock(ip);
    if (ip->type == T_DIR) {
        iunlockput(ip);
        end_op();
        return -EPERM;
    }

    ip->nlink++;
    iupdate(ip);
    iunlock(ip);

    if ((dp = nameiparent(new, name)) == 0) {
        error = -ENOENT;
        goto bad;
    }

    ilock(dp);
    if (dp->dev != ip->dev) {
        error = -EXDEV;
        iunlockput(dp);
        goto bad;
    }

    if ((error = dirlink(dp, name, ip->inum)) != 0) {
        iunlockput(dp);
        goto bad;
    }

    iunlockput(dp);
    iput(ip);
    end_op();
    return 0;

bad:
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return error;
}

long
filesymlink(char *old, char *new)
{
    char name[DIRSIZ];
    struct inode *dp, *ip;
    long error;
    struct timespec ts;

    begin_op();
    if ((dp = nameiparent(new, name)) == 0) {
        end_op();
        return -ENOENT;
    }

    ilock(dp);
    if ((ip = ialloc(dp->dev, T_SYMLINK)) == 0) {
        iunlockput(dp);
        end_op();
        return -ENOMEM;
    }

    ilock(ip);
    ip->major = 0;
    ip->minor = 0;
    ip->nlink = 1;
    ip->mode = S_IFLNK | 0777;
    ip->type = T_SYMLINK;
    clock_gettime(CLOCK_REALTIME, &ts);
    ip->atime = ip->mtime = ip->ctime = ts;
    iupdate(ip);

    if ((error = dirlink(dp, name, ip->inum)) != 0) {
        iunlockput(dp);
        iunlockput(ip);
        end_op();
        return error;
    }

    iupdate(dp);
    iunlockput(dp);

    writei(ip, old, 0, strlen(old));
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return 0;
}

ssize_t
filereadlink(char *path, char *buf, size_t bufsize)
{
    struct inode *ip, *dp;
    ssize_t n;
    char name[DIRSIZ];

    begin_op();
    if ((dp = nameiparent(path, name)) == 0) {
        end_op();
        return -ENOTDIR;
    } else {
        iput(dp);
    }

    if ((ip = namei(path)) == 0) {
        end_op();
        return -ENOENT;
    }

    ilock(ip);
    if (ip->type != T_SYMLINK) {
        iunlockput(ip);
        end_op();
        return -EINVAL;
    }

    if ((n = readi(ip, buf, 0, bufsize)) <= 0) {
        iunlockput(ip);
        end_op();
        return -EIO;
    }

    iunlockput(ip);
    end_op();
    return n;
}

long
fileunlink(char *path)
{
    struct inode *ip, *dp;
    char name[DIRSIZ];
    size_t off;
    long error;

    begin_op();
    if ((dp = nameiparent(path, name)) == 0) {
        end_op();
        return -ENOENT;
    }

    ilock(dp);

    /* Cannot unlink "." or "..". */
    if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0) {
        error = -EPERM;
        goto bad;
    }

    if ((ip = dirlookup(dp, name, &off)) == 0) {
        error = -ENOENT;
        goto bad;
    }

    ilock(ip);

    if (ip->nlink < 1)
        panic("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip)) {
        iunlockput(ip);
        error = -EPERM;
        goto bad;
    }

    if (unlink(dp, off) < 0)
        panic("unlink: unlink");

    if (ip->type == T_DIR) {
        dp->nlink--;
        iupdate(dp);
    }
    iunlockput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return 0;

bad:
    iunlockput(dp);
    end_op();
    return error;

}

/*
 * Allocate a file descriptor for the given file.
 * Takes over file reference from caller on success.
 */
long
fdalloc(struct file *f, int from)
{
    struct proc *p = thisproc();

    for (int fd = from; fd < NOFILE; fd++) {
        if (p->ofile[fd] == 0) {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -EBADF;
}

long
utimensat(char *path, struct timespec times[2])
{
    struct inode *ip, *dp;
    char name[DIRSIZ];
    size_t off;
    struct timespec ts;

    begin_op();
    if ((dp = nameiparent(path, name)) == 0) {
        end_op();
        return -ENOENT;
    }

    ilock(dp);
    if ((ip = dirlookup(dp, name, &off)) == 0) {
        iunlockput(dp);
        end_op();
        return -ENOENT;
    }
    iunlockput(dp);

    ilock(ip);
    // TODO: 権限チェックが必要
    if (times == NULL) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ip->atime = ip->mtime = ts;
    } else {
        for (int i = 0; i < 2; i++) {
            if (times[i].tv_nsec == UTIME_NOW) {
                clock_gettime(CLOCK_REALTIME, &ts);
                if (i == 0) ip->atime = ts;
                else        ip->mtime = ts;
            } else if (times[i].tv_nsec == UTIME_OMIT) {
                // noop
            } else {
                if (i == 0) ip->atime = times[i];
                else        ip->mtime = times[i];
            }
        }
    }
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return 0;
}
