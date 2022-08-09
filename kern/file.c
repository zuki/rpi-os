/* File descriptors */

#include "linux/errno.h"

#include "types.h"
#include "vfs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "console.h"
#include "log.h"
#include "pipe.h"
#include "clock.h"
#include "string.h"
#include "pagecache.h"
#include "vfsmount.h"
#include "linux/stat.h"
#include "linux/capability.h"

struct devsw devsw[NMAJOR];
struct {
    struct spinlock lock;
    struct file file[NFILE];
} ftable;

/* Optional since BSS is zero-initialized. */
void
fileinit()
{
    initlock(&ftable.lock, "file");
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
    long error;

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
            ip->iops->ilock(ip);
        }
    } else {
loop:
        if ((ip = namei(path)) == 0) {
            end_op();
            warn("cant namei %s", path);
            return -ENOENT;
        }
        ip->iops->ilock(ip);
        if (ip->type == T_DIR && (flags & O_ACCMODE) != 0) {
            iunlockput(ip);
            end_op();
            warn("wrong flags 0x%llx", flags);
            return -EINVAL;
        }
        if (ip->type == T_SYMLINK) {
            if ((n = ip->iops->readi(ip, buf, 0, sizeof(buf) - 1)) <= 0) {
                iunlockput(ip);
                end_op();
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
    if (readable && ((error = ip->iops->permission(ip, MAY_READ)) < 0))
        goto bad;
    if (writable && ((error = ip->iops->permission(ip, MAY_WRITE)) < 0))
        goto bad;
    if ((f = filealloc()) == 0 || (fd = fdalloc(f, 0)) < 0) {
        ip->iops->iunlock(ip);
        end_op();
        if (f) fileclose(f);
        warn("cant alloc file\n");
        return -ENOSPC;
    }
    ip->iops->iunlock(ip);
    end_op();

    f->type     = FD_INODE;
    f->ip       = ip;
    f->off      = (flags & O_APPEND) ? ip->size : 0;
    f->flags    = flags;
    f->readable = readable;
    f->writable = writable;
    if (flags & O_CLOEXEC)
        bit_add(thisproc()->fdflag, fd);
    debug("proc[%d], path: %s, fd: %d, flags: %d, mode: 0x%x", thisproc()->pid, path, fd, f->flags, f->ip->mode);
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
        f->ip->iops->ilock(f->ip);
        f->ip->iops->stati(f->ip, st);
        f->ip->iops->iunlock(f->ip);
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
        f->ip->iops->ilock(f->ip);
        if ((r = f->ip->iops->readi(f->ip, addr, f->off, n)) > 0)
            f->off += r;
        clock_gettime(CLOCK_REALTIME, &f->ip->atime);
        f->ip->iops->iunlock(f->ip);
        end_op();
        return r;
    }
    panic("fileread");
    return -EINVAL;
}

long
filepread64(struct file *f, void *buf, size_t count, off_t offset)
{
    long error;
    off_t old_off;
    ssize_t n;

    if ((old_off = filelseek(f, 0, SEEK_CUR)) < 0)
        return old_off;
    if ((error = filelseek(f, offset, SEEK_SET)) < 0)
        return error;
    n = fileread(f, (char *)buf, count);
    if ((error = filelseek(f, old_off, SEEK_SET)) < 0)
        return error;
    return n;
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
         * 最大ログトランザクションサイズを超えないように、
         * 一度に数ブロックずつ書き込む。対象ブロックには
         * i-node、間接ブロック、アロケーションブロック、
         * 非アライン書き込み用の2ブロックのスロップがある。
         * writei() はコンソールのようなデバイスに書き込む
         * 場合もあるのでこれは本当に最下層のことです。
         */
        ssize_t max = ((MAXOPBLOCKS - 1 - 2 - 2) / 2) * 512;
        ssize_t i = 0;
        while (i < n) {
            ssize_t n1 = MIN(max, n - i);
            begin_op();
            f->ip->iops->ilock(f->ip);
            if ((r = f->ip->iops->writei(f->ip, addr + i, f->off, n1)) > 0
             && (f->ip->type == T_FILE)) {
                update_page(f->off, f->ip->inum, f->ip->dev, addr + i, r);
                f->off += r;
             }
            clock_gettime(CLOCK_REALTIME, &ts);
            f->ip->mtime = f->ip->atime = ts;
            f->ip->iops->iunlock(f->ip);
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
    debug("invalid offset %d", offset)
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

    ip->iops->ilock(ip);
    if (ip->type == T_DIR) {
        iunlockput(ip);
        end_op();
        return -EPERM;
    }

    ip->nlink++;
    ip->iops->iupdate(ip);
    ip->iops->iunlock(ip);

    if ((dp = nameiparent(new, name)) == 0) {
        error = -ENOENT;
        goto bad;
    }

    dp->iops->ilock(dp);
    if (dp->dev != ip->dev) {
        error = -EXDEV;
        iunlockput(dp);
        goto bad;
    }

    if ((error = dp->iops->dirlink(dp, name, ip->inum, ip->type)) != 0) {
        iunlockput(dp);
        goto bad;
    }

    iunlockput(dp);
    iput(ip);
    end_op();
    return 0;

bad:
    ip->iops->ilock(ip);
    ip->nlink--;
    ip->iops->iupdate(ip);
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

    dp->iops->ilock(dp);
    if ((ip = dp->fs_t->ops->ialloc(dp->dev, T_SYMLINK)) == 0) {
        iunlockput(dp);
        end_op();
        return -ENOMEM;
    }

    ip->iops->ilock(ip);
    ip->major = 0;
    ip->minor = 0;
    ip->nlink = 1;
    ip->mode = S_IFLNK | 0777;
    ip->type = T_SYMLINK;
    clock_gettime(CLOCK_REALTIME, &ts);
    ip->atime = ip->mtime = ip->ctime = ts;
    ip->iops->iupdate(ip);

    if ((error = dp->iops->dirlink(dp, name, ip->inum, ip->type)) != 0) {
        iunlockput(dp);
        iunlockput(ip);
        end_op();
        return error;
    }

    dp->iops->iupdate(dp);
    iunlockput(dp);

    ip->iops->writei(ip, old, 0, strlen(old));
    ip->iops->iupdate(ip);
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

    ip->iops->ilock(ip);
    if (ip->type != T_SYMLINK) {
        iunlockput(ip);
        end_op();
        return -EINVAL;
    }

    if ((n = ip->iops->readi(ip, buf, 0, bufsize)) <= 0) {
        iunlockput(ip);
        end_op();
        return -EIO;
    }

    iunlockput(ip);
    end_op();
    return n;
}

long
fileunlink(char *path, int flags)
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

    dp->iops->ilock(dp);

    /* Cannot unlink "." or "..". */

    if (dp->fs_t->ops->namecmp(name, ".") == 0 || dp->fs_t->ops->namecmp(name, "..") == 0) {
        error = -EPERM;
        goto baddp;
    }

    if ((ip = dp->iops->dirlookup(dp, name, &off)) == 0) {
        error = -ENOENT;
        goto baddp;
    }

    ip->iops->ilock(ip);

    if (ip->nlink < 1) {
        return -EPERM;
        goto badip;
        //panic("unlink: nlink < 1");
    }

    if (flags & AT_REMOVEDIR) {
        if (ip->type != T_DIR) {
            error = -ENOTDIR;
            goto badip;
        }
        if (!ip->iops->isdirempty(ip)) {
            error = -EPERM;
            goto badip;
        }
    } else {
        if (ip->type == T_DIR) {
            error = -EISDIR;
            goto badip;
        }
    }

    if (dp->iops->unlink(dp, off) < 0) {
        error = -EIO;
        goto badip;
        //panic("unlink: unlink");
    }

    if (ip->type == T_DIR) {
        dp->nlink--;
        dp->iops->iupdate(dp);
    }
    iunlockput(dp);

    ip->nlink--;
    ip->iops->iupdate(ip);
    iunlockput(ip);
    end_op();
    return 0;

badip:
    iunlockput(ip);
baddp:
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

    dp->iops->ilock(dp);
    if ((ip = dp->iops->dirlookup(dp, name, &off)) == 0) {
        iunlockput(dp);
        end_op();
        return -ENOENT;
    }
    iunlockput(dp);

    ip->iops->ilock(ip);
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
    ip->iops->iupdate(ip);
    iunlockput(ip);
    end_op();
    return 0;
}

long
filechmod(char *path, mode_t mode)
{
    struct inode *ip;

    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -ENOENT;
    }

    ip->iops->ilock(ip);
    ip->mode = (ip->mode & S_IFMT) | mode;
    ip->iops->iupdate(ip);
    iunlockput(ip);
    end_op();

    return 0;
}

long
filechown(struct file *f, char *path, uid_t owner, gid_t group)
{
    struct inode *ip;
    struct proc *p = thisproc();
    long error = -EPERM;
    int i;

    begin_op();
    if (f != NULL) {
        ip = f->ip;
    } else {
        if ((ip = namei(path)) == 0) {
            end_op();
            return -ENOENT;
        }
    }

    ip->iops->ilock(ip);

    if (owner != (uid_t)-1) {
        if (!capable(CAP_CHOWN)) {
            debug("uid %d cant chown", owner);
            goto bad;
        }
        ip->uid = owner;
    }

    if (group != (gid_t)-1) {
        if (capable(CAP_CHOWN)) {
            ip->gid = group;
        } else if (ip->uid == p->euid) {
            for (i = 0; i < p->ngroups; i++) {
                if (p->groups[i] == group) {
                    ip->gid = group;
                    break;
                }
            }
            if (i == p->ngroups) {
                debug("uid %d and gid %d cant chown", owner, group);
                goto bad;
            }
        } else {
            debug("gid %d cant chown", group);
            goto bad;
        }
    }

    if (ip->mode & S_IXUGO && !capable(CAP_CHOWN)) {
        ip->mode &= ~(S_ISUID|S_ISGID);
    }
    ip->iops->iupdate(ip);
    error = 0;

bad:
    iunlockput(ip);
    end_op();
    return error;
}

long
faccess(char *path, int mode, int flags)
{
    struct inode *ip;
    struct proc *p = thisproc();
    uid_t uid, fuid;
    gid_t gid, fgid;
    mode_t fmode;

    if (mode & ~S_IRWXO)    /* where's F_OK, X_OK, W_OK, R_OK? */
        return -EINVAL;

    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -ENOENT;
    }
    fmode = ip->mode;
    fuid = ip->uid;
    fgid = ip->gid;
    iput(ip);
    end_op();

    // mode == F_OKの場合はファイルが存在するのでOK
    if (mode == 0) return 0;

    // 呼び出し元がrootならR_OK, W_OKは常にOK
    // X_OKはファイルにUGOのいずれかに実行許可があればOK
    if (p->uid == 0) {
        if ((mode & X_OK) && !(fmode & S_IXUGO))
            return -EACCES;
        else
            return 0;
    }

    // root以外は個別に判断
    if (flags & AT_EACCESS) {       // 実効IDで判断
        uid = p->euid;
        gid = p->egid;
    } else {                        // 実IDで判断
        uid = p->uid;
        gid = p->gid;
    }

    if (mode & R_OK) {
        if ((fuid == uid && !(fmode & S_IRUSR))
         && (fgid == gid && !(fmode & S_IRGRP))
                         && !(fmode & S_IROTH))
        return -EACCES;
    }

    if (mode & W_OK) {
        if ((fuid == uid && !(fmode & S_IWUSR))
         && (fgid == gid && !(fmode & S_IWGRP))
                         && !(fmode & S_IWOTH))
        return -EACCES;
    }

     if (mode & X_OK) {
        if ((fuid == uid && !(fmode & S_IXUSR))
         && (fgid == gid && !(fmode & S_IXOTH))
                         && !(fmode & S_IXOTH))
        return -EACCES;
    }

    return 0;
}

// dp/name1 -> dp/name2 へ改名
static long
rename(struct inode *dp, char *name1, char *name2)
{
    struct inode *ip;
    struct dirent de;
    size_t off;

    dp->iops->ilock(dp);
    if ((ip = dp->iops->dirlookup(dp, name1, &off)) == 0)
        return -ENOENT;
    ip->iops->ilock(ip);

    memset(&de, 0, sizeof(de));
    de.inum = ip->inum;
    de.type = ip->type;
    memmove(de.name, name2, DIRSIZ);
    if (dp->iops->writei(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) {
        warn("writei");
        return -ENOSPC;
    }
    dp->iops->iupdate(dp);
    clock_gettime(CLOCK_REALTIME, &ip->ctime);
    ip->iops->iupdate(ip);
    iunlockput(ip);
    iunlockput(dp);

    return 0;
}

// dp/old_ip -> dp/new_ip へ付け替え
static long
reinode(struct inode *dp, struct inode *old_ip, struct inode *new_ip)
{
    struct dirent de;
    size_t off;
    struct timespec ts;

    dp->iops->ilock(dp);
    old_ip->iops->ilock(old_ip);
    new_ip->iops->ilock(new_ip);
    if (dp->fs_t->ops->direntlookup(dp, old_ip->inum, &de, &off) < 0)
        return -ENOENT;

    de.inum = new_ip->inum;
    if (dp->iops->writei(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) {
        warn("writei");
        return -ENOSPC;
    }
    dp->iops->iupdate(dp);
    clock_gettime(CLOCK_REALTIME, &ts);
    old_ip->ctime = new_ip->ctime = ts;
    old_ip->iops->iupdate(old_ip);
    new_ip->iops->iupdate(new_ip);
    iunlockput(old_ip);
    iunlockput(new_ip);
    iunlockput(dp);

    return 0;
}

long
filerename(char *path1, char *path2)
{
    struct inode *ip1, *ip2, *dp1, *dp2;
    char name1[DIRSIZ], name2[DIRSIZ];
    long error;

    begin_op();
    if ((ip1 = namei(path1)) == 0) {
        end_op();
        warn("path1 %s not exits", path1);
        return -ENOENT;
    }
    dp1 = nameiparent(path1, name1);

    ip2 = namei(path2);
    dp2 = nameiparent(path2, name2);
    debug("path1: %s, dp1: %d, ip1: %d, name1: %s", path1, dp1->inum, ip1->inum, name1);
    debug("path2: %s, dp2: %d, ip2: %d, name2: %s", path2, dp2 ? dp2->inum : -1, ip2 ? ip2->inum : -1, name2);

    // 同一ファイルのhard link
    if (ip1 == ip2) return 0;

    error = -EINVAL;
    // 親ディレクトリが同じ
    if (dp1 == dp2) {
        // name2はなし: 単なる改名(name1はdirectoryでもfileでも可)
        if (ip2 == 0) {
            if ((error = rename(dp1, name1, name2)) < 0) {
                warn("rename failed");
                goto bad;
            }
        // name2あり: direentを付け替えて、ip2はunlink
        } else {
            if ((error = reinode(dp2, ip2, ip1)) < 0) {
                warn("reinode failed");
                goto bad;
            }
            return fileunlink(path2, ip2->type == T_DIR ? AT_REMOVEDIR : 0);
        }
    // 異なるディレクトリへのmove
    } else {
        if (ip2 == 0) {
            if ((error = dp2->iops->dirlink(dp2, name2, ip1->inum, ip1->type)) < 0) {
                warn("dirlink failed 2");
                goto bad;
            }
            end_op();
            return fileunlink(path1, ip1->type == T_DIR ? AT_REMOVEDIR : 0);
        } else {
            if ((error = reinode(dp2, ip2, ip1)) < 0) {
                warn("reinode failed 2");
                goto bad;
            }
            end_op();
            return fileunlink(path2, ip2->type == T_DIR ? AT_REMOVEDIR : 0);
        }
    }
    error = 0;

bad:
    end_op();
    return error;
}

long
mount(char *source, char *target, char *fstype, uint64_t flags, void *data)
{
    struct inode *ip, *devi = 0;
    struct filesystem_type *fs_t;
    long error;
    info("source: %s, target: %s, type: %s, flags: 0x%llx", source, target, fstype, flags);
    begin_op();
    if ((ip = namei(target)) == 0 || (devi = namei(source)) == 0) {
        warn("not found: ip=0x%p, devi=0x%p", ip, devi);
        end_op();
        return -ENOENT;
    }

    // 1: fstypeがサポートされているかチェック
    if ((fs_t = getfs(fstype)) == 0) {
        warn("fstype %s not found", fstype);
        return -ENODEV;
    }

    error = -ENOTDIR;
    ip->iops->ilock(ip);
    devi->iops->ilock(devi);

    // 2: マウントポイント ipの妥当性チェック: directoryであること
    if (ip->type != T_DIR && ip->ref > 1) {
        warn("target %s is not dir", target);
        goto bad;
    }

    // 3: 被マウント deviの妥当性チェック: デバイスファイルであること
    error = -ENOTBLK;
    if (devi->type != T_DEV) {
        warn("source %s is not dev", source);
        goto bad;
    }

    // 4: デバイスのオープン（当面SDカードのみで常時オープンのためチェックしない）
/*
    error = -EINVAL;
    if (bdev_open(devi) != 0) {
        warn("source could not opened\n");
        goto bad;
    }
*/
    // 5: デバイスファイルの妥当性: VFATでもルートデバイスでもないこと
    if (devi->minor == FATMINOR || devi->minor == ROOTDEV) {
        warn("dev %d is vfat or rootdev", devi->minor);
        goto bad;
    }

    // 6: vfsリストにデバイスとファイルシステムを登録
    // Add this to a list to retrieve the filesystem type to current device
    if (putvfsonlist(devi->major, devi->minor, fs_t) == -1) {
        warn("failed to add dev to list");
        goto bad;
    }

    // 7: ファイルシステム固有のマウント操作
    if (fs_t->ops->mount(devi, ip) != 0) {
        warn("failed to mount\n");
        goto bad;
    }

    // 8: マウントポイントのファイルタイプを変更
    ip->type = T_MOUNT;
    //ip->iops->iupdate(ip);    // これを活かすとconsoleでcntl-Dが効かなくなる
    devi->iops->iunlock(devi);
    ip->iops->iunlock(ip);
    end_op();
    return 0;

bad:
    iunlockput(devi);
    iunlockput(ip);
    end_op();
    return error;
}

long
umount(char *target, int flags)
{
    struct inode *ip, *devi;
    struct vfs *vfs;
    long error;

    begin_op();
    if ((devi = namei(target)) == 0) {
        warn("target %s is not found", target);
        end_op();
        return -ENOENT;
    }

    devi->iops->ilock(devi);
    if ((ip = mtablemntinode(devi)) == 0) {
        warn("could not get device ip");
        error = -EINVAL;
        goto bad2;
    }
    ip->iops->ilock(ip);

    if ((vfs = getvfsentry(SDMAJOR, devi->dev)) == 0) {
        warn("could not get vfsentry");
        error = -ENOENT;
        goto bad1;
    }

    if (vfs->fs_t->ops->unmount(devi) < 0) {
        warn("could not unmount");
        error = -EINVAL;
        goto bad1;
    }

    if ((error = deletefsfromlist(vfs)) < 0) {
        warn("error on delete fs from mlist");
        goto bad1;
    }

    devi->ref--;
    ip->type = T_DIR;

    error = 0;

bad1:
    iunlockput(ip);
bad2:
    iunlockput(devi);
    end_op();
    return error;
}
