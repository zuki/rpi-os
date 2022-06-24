#ifndef INC_LINUX_STAT_H
#define INC_LINUX_STAT_H

#include "linux/time.h"
#include "types.h"

#define S_IFMT   00170000

#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000
#define S_IRUSR  0400
#define S_IWUSR  0200
#define S_IXUSR  0100
#define S_IRWXU  0700
#define S_IRGRP  0040
#define S_IWGRP  0020
#define S_IXGRP  0010
#define S_IRWXG  0070
#define S_IROTH  0004
#define S_IWOTH  0002
#define S_IXOTH  0001
#define S_IRWXO  0007

#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#define S_ISCHR(mode)  (((mode) & S_IFMT) == S_IFCHR)
#define S_ISBLK(mode)  (((mode) & S_IFMT) == S_IFBLK)
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define S_ISLNK(mode)  (((mode) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)

#define S_IRWXUGO   (S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO   (S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO     (S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO     (S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO     (S_IXUSR|S_IXGRP|S_IXOTH)

#define UTIME_NOW  0x3fffffff
#define UTIME_OMIT 0x3ffffffe

#define T_DIR       1   // Directory
#define T_FILE      2   // File
#define T_DEV       3   // Device
#define T_MOUNT     4   // Mount Point
#define T_SYMLINK   5   // Sysbolic link

#define DT_UNKNOWN      0
#define DT_FIFO         1
#define DT_CHR          2
#define DT_DIR          4
#define DT_BLK          6
#define DT_REG          8
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14
#define IFTODT(x)       ((x)>>12 & 017)
#define DTTOIF(x)       ((x)<<12)

struct stat {
        dev_t st_dev;
        ino_t st_ino;
        mode_t st_mode;
        nlink_t st_nlink;
        uid_t st_uid;
        gid_t st_gid;
        dev_t st_rdev;
        unsigned long __pad;
        off_t st_size;
        blksize_t st_blksize;
        int __pad2;
        blkcnt_t st_blocks;
        struct timespec st_atime;
        struct timespec st_mtime;
        struct timespec st_ctime;
        unsigned __unused[2];
};

#define major(x) \
        ((unsigned)( (((x)>>31>>1) & 0xfffff000) | (((x)>>8) & 0x00000fff) ))
#define minor(x) \
        ((unsigned)( (((x)>>12) & 0xffffff00) | ((x) & 0x000000ff) ))

#define makedev(x,y) ( \
        (((x)&0xfffff000ULL) << 32) | \
        (((x)&0x00000fffULL) << 8) | \
        (((y)&0xffffff00ULL) << 12) | \
        (((y)&0x000000ffULL)) )

#endif
