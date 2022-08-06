#ifndef INC_FILE_H
#define INC_FILE_H

#include "types.h"
#include "sleeplock.h"
#include "vfs.h"
#include "linux/fcntl.h"
#include "linux/ioctl.h"
#include "linux/stat.h"

#define NFILE 512  // Open files per system

#define UIO_MAXIOV 1024

#define FILE_STATUS_FLAGS (O_APPEND|O_ASYNC|O_DIRECT|O_DSYNC|O_NOATIME|O_NONBLOCK|O_SYNC)
#define FILE_READABLE(flags) ((((flags) & O_ACCMODE) == O_RDWR) || (((flags) & O_ACCMODE) == O_RDONLY));
#define FILE_WRITABLE(flags) ((((flags) & O_ACCMODE) == O_RDWR) || (((flags) & O_ACCMODE) == O_WRONLY));

struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE } type;
    int ref;
    struct pipe *pipe;
    struct inode *ip;
    off_t off;
    int flags;
    char readable;
    char writable;
};

/*
 * Table mapping major device number to
 * device functions
 */
struct devsw {
    ssize_t (*read)(struct inode *, char *, ssize_t);
    ssize_t (*write)(struct inode *, char *, ssize_t);
    struct termios *termios;
};

extern struct devsw devsw[];

struct file *   filealloc();
struct file *   filedup(struct file *f);
long            fileopen(char *path, int flags, mode_t mode);
void            fileclose(struct file *f);
long            filestat(struct file *f, struct stat *st);
ssize_t         fileread(struct file *f, char *addr, ssize_t n);
ssize_t         filewrite(struct file *f, char *addr, ssize_t n);
ssize_t         filelseek(struct file *f, off_t offset, int whence);
long            filelink(char *old, char *new);
long            fileunlink(char *path, int flags);
long            filesymlink(char *old, char *new);
ssize_t         filereadlink(char *path, char *buf, size_t bufsize);
long            filerename(char *path1, char *path2);
long            filechmod(char *path, mode_t mode);
long            filechown(struct file *f, char *path, uid_t owner, gid_t group);
long            filepread64(struct file *f, void *buf, size_t count, off_t offset);

long            fsync(struct file *f, int type);
long            fdalloc(struct file *f, int from);
long            faccess(char *paht, int mode, int flags);
long            utimensat(char *path, struct timespec times[2]);

long            mount(char *source, char *target, char *fstype, uint64_t flags, void *data);
long            umount(char *target, int flags);
#endif
