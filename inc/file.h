#ifndef INC_FILE_H
#define INC_FILE_H

#include "types.h"
#include "sleeplock.h"
#include "fs.h"
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
    size_t off;
    int flags;
    char readable;
    char writable;
};

/* In-memory copy of an inode. */
struct inode {
    uint32_t dev;               // Device number
    uint32_t inum;              // Inode number
    int ref;                    // Reference count
    struct sleeplock lock;      // Protects everything below here
    int valid;                  // Inode has been read from disk?

    uint16_t type;              // file type: Copy of disk inode
    uint16_t major;             // Major device number (T_DEV only)
    uint16_t minor;             // Minor device number (T_DEV only)
    uint16_t nlink;             // Number of links to inode in file system
    uint32_t size;              // Size of file (bytes)

    mode_t mode;                // file mode
    uid_t  uid;                 // Owner's user id
    gid_t  gid;                 // Owner's group id
    struct timespec atime;      // last accessed time
    struct timespec mtime;      // last modified time
    struct timespec ctime;      // created time

    uint32_t addrs[NDIRECT+2];
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

void            readsb(int, struct superblock *);
int             dirlink(struct inode *, char *, uint32_t);
struct inode *  dirlookup(struct inode *, char *, size_t *);
int             direntlookup(struct inode *dp, int inum, struct dirent *dep);
struct inode *  ialloc(uint32_t, short);
struct inode *  idup(struct inode *);
void            iinit(int dev);
void            ilock(struct inode *);
void            iput(struct inode *);
void            iunlock(struct inode *);
void            iunlockput(struct inode *);
void            iupdate(struct inode *);
int             unlink(struct inode *dp, uint32_t off);
int             namecmp(const char *, const char *);
struct inode *  namei(const char *);
struct inode *  nameiparent(const char *, char *);
void            stati(struct inode *, struct stat *);
ssize_t         readi(struct inode *, char *, size_t, size_t);
ssize_t         writei(struct inode *, char *, size_t, size_t);
struct inode *  create(char *path, short type, short major, short minor, mode_t mode);
long            getdents(struct file *f, char *data, size_t size);
int             permission(struct inode *ip, int mask);

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

long            fsync(struct file *f, int type);
long            fdalloc(struct file *f, int from);
long            faccess(char *paht, int mode, int flags);
long            utimensat(char *path, struct timespec times[2]);

#endif
