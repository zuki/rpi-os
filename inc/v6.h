/*
 * On-disk file system format.
 * Both the kernel and user programs use this header file.
 */
#ifndef INC_FS_H
#define INC_FS_H

#include "types.h"
#include "vfs.h"
#include "linux/fcntl.h"
#include "linux/time.h"

// Belows are used by both
#define LOGSIZE         (MAXOPBLOCKS*3)     // Max data blocks in on-disk log
#define BSIZE           4096                // Block size

/* Disk layout:
 * [ boot block | super block | log | inode blocks | free bit map | data blocks ]
 *
 * mkfs computes the super block and builds an initial file system. The
 * super block describes the disk layout:
 */
struct v6_superblock {
    uint32_t size;         // Size of file system image (blocks)
    uint32_t nblocks;      // Number of data blocks
    uint32_t ninodes;      // Number of inodes.
    uint32_t nlog;         // Number of log blocks
    uint32_t logstart;     // Block number of first log block
    uint32_t inodestart;   // Block number of first inode block
    uint32_t bmapstart;    // Block number of first free map block
};

#define V6_SB_FREE      0
#define V6_SB_USED      1

#define NDIRECT         11
#define NINDIRECT       (BSIZE / sizeof(uint32_t))                      // 1024
#define MAXFILE         (NDIRECT + NINDIRECT + NINDIRECT * NINDIRECT)   // (11 + 1024 + 1024 * 1024) * 4096 = 4100 MB
#define V6_ROOTINO      1

struct v6_inode {
    uint32_t flag;                // V6_INODE_FREE/V6_INODE_USED
    uint32_t addrs[NDIRECT+2];    // Data block addresses
};

/* On-disk inode structure. */
struct dinode {
    uint16_t type;                // File type
    uint16_t major;               // Major device number (T_DEV only)
    uint16_t minor;               // Minor device number (T_DEV only)
    uint16_t nlink;               // Number of links to inode in file system
    uint32_t size;                // Size of file (bytes)
    mode_t mode;                  // file mode
    uid_t    uid;                 // owner's user id
    gid_t    gid;                 // owner's gropu id
    struct timespec atime;        // last accessed time
    struct timespec mtime;        // last modified time
    struct timespec ctime;        // created time
    uint32_t addrs[NDIRECT+2];    // Data block addresses
    char    _dummy[4];
};

/* Inodes per block. */
#define IPB           (BSIZE / sizeof(struct dinode))   // 32 = 4096 / 128

/* Block containing inode i. */
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

/* Bitmap bits per block. */
#define BPB           (BSIZE*8)                         // 32,768

/* Block of free map containing bit for block b. */
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)

// Filesystem operations
int             v6fs_init(void);
int             v6_mount(struct inode *devi, struct inode *ip);
int             v6_unmount(struct inode *devi);
struct inode *  v6_getroot(int major, int minor);
void            v6_readsb(uint32_t dev, struct superblock *sb);
struct inode *  v6_ialloc(uint32_t dev, uint16_t type);
uint32_t        v6_balloc(uint32_t dev);
void            v6_bzero(uint32_t dev, int bno);
void            v6_bfree(uint32_t dev, uint32_t b);
int             v6_namecmp(const char *s, const char *t);
int             v6_direntlookup(struct inode *dp, int inum, struct dirent *dep, size_t *ofp);
uint16_t        v6_getrootino(void);
long            v6_getdents(struct file *f, char *data, size_t size);

// inode operations
struct inode *  v6_dirlookup(struct inode *dp, char *name, size_t *poff);
void            v6_iupdate(struct inode *ip);
void            v6_itrunc(struct inode *ip);
void            v6_cleanup(struct inode *ip);
uint32_t        v6_bmap(struct inode *ip, uint32_t bn);
void            v6_ilock(struct inode *ip);
ssize_t         v6_readi(struct inode *ip, char *dst, size_t off, size_t n);
ssize_t         v6_writei(struct inode *ip, char *src, size_t off, size_t n);
int             v6_unlink(struct inode *dp, uint32_t off);
int             v6_isdirempty(struct inode *dp);

// V6 Specific function
int             init_v6fs(void);
#endif
