#ifndef USR_INC_PARAM_H
#define USR_INC_PARAM_H

#define NINODE      1024                // maximum number of active i-nodes
#define NDEV        10                  // maximum major device number
#define MAXBDEV     4                   // maximum numbers of block devices
#define ROOTDEV     1                   // device number of file system root disk
#define MAXARG      32                  // maximu number of exec arguments
#define MAXOPBLOCKS 42                  // maximum number of blocks any FS op writes
#define NBUF        (MAXOPBLOCKS*3)     // size of disk block cache
#define FSSIZE      100000              // size of file system in blocks 400MB
#define MAXVFSSIZE  4                   // maximum number of vfs fils systems
#define SDMAJOR     0                   // SD card major block device
#define CONMAJOR    1                   // Console device
#define ROOTFSTYPE  "v6"                //
#define MAXBSIZE    4096                // maximum BSIZE
#define NGROUPS     32                  // maxinum groups that user can belong to

#define NLOG        1                   // Max number of active logs
#define LOGENABLED  1
#define LOGSIZE     (NLOG*MAXOPBLOCKS*3)    // maximu number of data blocks in on-disk log

// Belows are used by both
#define ROOTDEV     1                   // Device number of file system root disk
#define ROOTINO     1                   // Root i-number
#define BSIZE       4096                // Block size
#define NDIRECT     11
#define NINDIRECT   (BSIZE / sizeof(uint32_t))  // 4096 / 4 = 1024
#define MAXFILE     (NDIRECT + NINDIRECT + NINDIRECT * NINDIRECT) // Max file size =4100 MB

#endif
