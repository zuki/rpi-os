/*
 * On-disk file system format.
 * Both the kernel and user programs use this header file.
 */
#ifndef INC_FS_H
#define INC_FS_H

#include "types.h"
#include "linux/fcntl.h"
#include "linux/time.h"

// Kernel only
#define NDEV            10                  // Maximum major device number
#define MAXBDEV         4                   // maximum numbers of block devices
#define NINODE          1024                // Maximum number of active i-nodes
#define MAXOPBLOCKS     42                  // Max # of blocks any FS op writes
#define NBUF            (MAXOPBLOCKS*3)     // Size of disk block cache
#define MAXVFSSIZE      4                   // maximum number of vfs fils systems
#define SDMAJOR         0                   // SD card major block device
#define CONMAJOR        1                   // Console device
#define ROOTFSTYPE      "v6"                //
#define MAXBSIZE        4096                // maximum BSIZE

// mkfs only
#define FSSIZE          100000              // Size of file system in blocks
                                            // fs.img のサイズ (BSIZE * FSSIZE = 409,600,000)

// Belows are used by both
#define LOGSIZE         (MAXOPBLOCKS*3)     // Max data blocks in on-disk log
#define ROOTDEV         1                   // Device number of file system root disk
#define ROOTINO         1                   // Root i-number

#define BSIZE           4096                // Block size

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

#define SB_NOT_LOADED   0
#define SB_INITIALIZED  1

#define SB_FREE         0
#define SB_USED         1

#define MAY_EXEC    1
#define MAY_WRITE   2
#define MAY_READ    4

#define NDIRECT 11
#define NINDIRECT (BSIZE / sizeof(uint32_t))    // 1024
#define MAXFILE (NDIRECT + NINDIRECT + NINDIRECT * NINDIRECT)   // (11 + 1024 + 1024 * 1024) * 4096 = 4100 MB

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

/* Directory is a file containing a sequence of dirent structures. */
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

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

#endif
