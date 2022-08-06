#ifndef INC_VFS_H
#define INC_VFS_H

#include "types.h"
#include "buf.h"
#include "list.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "linux/stat.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define ALIGN(p, n) (((p) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))

// Kernel only
#define NMAJOR          4                   // Maximum # of Major Device Number
#define NMINOR          10                  // Maximum # of Minor Device Number

#define SDMAJOR         0                   // SD card major block device
#define FATMINOR        0                   // FAT partition [0,0]
#define V6MINOR         1                   // V6 partition [0.1]
#define EXT2MINOR       2                   // Ext2 partition [0.2]

#define CONMAJOR        1                   // Console device

#define ROOTDEV         V6MINOR             // Root device #

#define MAXOPBLOCKS     42                  // Max # of blocks any FS op writes
#define NBUF            (MAXOPBLOCKS*3)     // Size of disk block cache
#define MAXVFSSIZE      4                   // maximum number of vfs fils systems
#define ROOTFSTYPE      "v6"                //
#define MAXBSIZE        4096                // maximum BSIZE
#define NINODE          1024                // Maximum # of active i-nodes
#define ROOTINO         1                   // Root i-number

// mkfs only
#define FSSIZE          100000              // Size of file system in blocks
                                            // fs.img のサイズ (BSIZE * FSSIZE = 409,600,000)

// bootレコードのパーティションテーブル
struct ptable {
    char flag;
    char chs1[3];
    char type;
    char chs2[3];
    uint32_t lba;
    uint32_t nsecs;
} __attribute__((packed));

// Master Boot Record
struct mbr {
    char _bootstrap[446];
    struct ptable ptables[4];
    uint16_t signature;
} __attribute__((packed));

extern struct mbr mbr;

struct superblock {
    int      major;             // Driver major number from it superblocks is stored in.
    int      minor;             // Driver minor number from it superblocks is stored in.
    uint32_t blocksize;         // Block size of this superblock
    uint32_t lba;
    uint32_t nsecs;
    void    *fs_info;           // Filesystem-specific info
    uint8_t  s_blocksize_bits;  //
    int      flags;             // Superblock flags to map its usage
};

extern struct superblock sb[NMINOR];
extern int ndev;

#define SB_NOT_LOADED   0
#define SB_INITIALIZED  1

#define SB_FREE         0
#define SB_USED         1

#define MAY_EXEC    1
#define MAY_WRITE   2
#define MAY_READ    4

struct inode_operations {
    struct inode *(*dirlookup)(struct inode *dp, char *name, size_t *off);
    void (*iupdate)(struct inode *ip);
    void (*itrunc)(struct inode *ip);
    void (*cleanup)(struct inode *ip);
    uint32_t (*bmap)(struct inode *ip, uint32_t bn);
    void (*ilock)(struct inode *ip);
    void (*iunlock)(struct inode *ip);
    void (*stati)(struct inode *ip, struct stat *st);
    ssize_t (*readi)(struct inode *ip, char *dst, size_t off, size_t n);
    ssize_t (*writei)(struct inode *ip, char *src, size_t off, size_t n);
    int (*dirlink)(struct inode *dp, char *name, uint32_t inum, uint16_t type);
    int (*unlink)(struct inode *dp, uint32_t off);
    int (*isdirempty)(struct inode *dp);
    int (*permission)(struct inode *ip, int mask);
};

// in-memory coy of an inode
struct inode {
    uint32_t dev;                       // Minor Divice number
    uint32_t inum;                      // Inode number
    int ref;                            // Reference count
    struct sleeplock lock;              // sleep lock
    int valid;                          // inode has been read from disk?
    struct filesystem_type *fs_t;       // THe filesystem type this inode is stored in
    struct inode_operations *iops;      // The specific inode operations
    void *i_private;                    // File system specific information

    uint16_t type;                      // File type
    uint16_t major;                     // Major device number (T_DEV only)
    uint16_t minor;                     // Minor device number (T_DEV only)
    uint16_t nlink;                     // Number of links to inode in file system
    uint32_t size;                      // Size of file (bytes)
    mode_t   mode;                      // File mode
    uid_t    uid;                       // owner's user id
    gid_t    gid;                       // owner's gropu id
    struct timespec atime;              // Last access time
    struct timespec mtime;              // Last modify time
    struct timespec ctime;              // Last create time
};

#define INODE_FREE  0
#define INODE_USED  1

struct icache {
    struct spinlock lock;
    struct inode inode[NINODE];
};
extern struct icache icache;

// DIrectory is a file containing a sequence of dirent structures.
#define DIRSIZ 58

struct dirent {
    uint32_t inum;
    uint16_t type;
    char name[DIRSIZ];
};

struct dirent64 {
  ino64_t d_ino;
  off64_t d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[];
};

struct vfs_operations {
    int             (*fs_init)(void);
    int             (*mount)(struct inode *, struct inode *);
    int             (*unmount)(struct inode *);
    struct inode *  (*getroot)(int, int);
    void            (*readsb)(uint32_t dev, struct superblock *sb);
    struct inode *  (*ialloc)(uint32_t dev, uint16_t type);
    uint32_t        (*balloc)(uint32_t dev);
    void            (*bzero)(uint32_t dev, int bno);
    void            (*bfree)(uint32_t dev, uint32_t b);
    void            (*brelse)(struct buf *b);
    void            (*bwrite)(struct buf *b);
    struct buf *    (*bread)(uint32_t dev, uint32_t blockno);
    int             (*namecmp)(const char *s, const char *t);
    int             (*direntlookup)(struct inode *dp, int inum, struct dirent *dep, size_t *ofp);
    uint16_t        (*getrootino)(void);
    long            (*getdents)(struct file *f, char *data, size_t size);
};

/*
 * This is struct is the map block device and its filesystem.
 * Its main job is return the filesystem type of current (major, minor)
 * mounted device. It is used when it is not possible retrieve the
 * filesystem_type from the inode.
 */
struct vfs {
    int major;
    int minor;
    int flag;
    struct filesystem_type *fs_t;
    struct list_head fs_next;       // Next mounted on vfs
};

#define VFS_FREE    0
#define VFS_USED    1

extern struct vfs *rootfs;         // It is the global pointer to root fs entry

/*
 * This is the representation of mounted lists.
 * It is different from the vfssw, because it is mapping the mounted
 * on filesystem per (major, minor).
 */
struct vfsmlist {
    struct spinlock lock;
    struct list_head fs_list;
};

extern struct vfsmlist vfsmlist;

struct filesystem_type {
    char                    *name;      // The filesystem name. It is used by the mount syscall
    struct vfs_operations   *ops;       // VFS operations
    struct inode_operations *iops;      // Pointer to inode operations of this FS
    struct list_head         fs_list;   // This is a list of filesystems used by vfssw
};

void        fs_init();
void        install_rootfs(void);
void        init_vfsmlist(void);
struct vfs *getvfsentry(int major, int minor);
int         putvfsonlist(int major, int minor, struct filesystem_type *fs_t);
int         deletefsfromlist(struct vfs *vfs);
void        init_vfssw(void);
int         register_fs(struct filesystem_type *fs);
struct filesystem_type *getfs(const char *fs_name);

// inode common operations
struct inode *iget(uint32_t dev, uint32_t inum, uint16_t type, int (*fill_super)(struct inode *));
struct inode *idup(struct inode *ip);
void          iinit(int dev);
void          iput(struct inode *ip);
void          iunlockput(struct inode * ip);
struct inode *namei(char *path);
struct inode *nameiparent(char *path, char *name);
struct inode *create(char *path, uint16_t type, uint16_t major, uint16_t minor, mode_t mode);

// Generic inode operations
void generic_iunlock(struct inode *ip);
void generic_stati(struct inode *ip, struct stat *st);
ssize_t  generic_readi(struct inode *ip, char *dst, size_t off, size_t n);
int  generic_dirlink(struct inode *dp, char *name, uint32_t inum, uint16_t type);
int  generic_permission(struct inode *ip, int mask);

int sb_set_blocksize(struct superblock *sb, uint32_t size);
int get_dev_FSLBA(uint32_t dev);

void print_dirent64(struct dirent64 *de);

#endif
