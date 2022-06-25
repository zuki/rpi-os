/*
 * On-disk file system format.
 * Both the kernel and user programs use this header file.
 */
#ifndef USR_INC_FS_H
#define USR_INC_FS_H

#include <stdint.h>
#include "param.h"

/* Disk layout:
 * [ boot block | super block | log | inode blocks | free bit map | data blocks ]
 *
 * mkfs computes the super block and builds an initial file system. The
 * super block describes the disk layout:
 */
struct superblock {
    uint32_t size;         // Size of file system image (blocks)
    uint32_t nblocks;      // Number of data blocks
    uint32_t ninodes;      // Number of inodes.
    uint32_t nlog;         // Number of log blocks
    uint32_t logstart;     // Block number of first log block
    uint32_t inodestart;   // Block number of first inode block
    uint32_t bmapstart;    // Block number of first free map block
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

/* Inodes per block: 4 */
#define IPB           (BSIZE / sizeof(struct dinode))

/* Block containing inode i. */
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

/* Bitmap bits per block;  4096 = 0x1000 */
#define BPB           (BSIZE*8)

/* Block of free map containing bit for block b. */
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)

/* Directory is a file containing a sequence of dirent structures. */
#define DIRSIZ 60

struct dirent {
  uint32_t inum;
  char name[DIRSIZ];
};

#define T_DIR        1   // Directory
#define T_FILE      2   // File
#define T_DEV       3   // Device
#define T_MOUNT     4   // Mount Point
#define T_SYMLINK   5   // Sysbolic link

#endif
