#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>

typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t uint;
typedef uint64_t ulong;

// this file should be compiled with normal gcc...

#define stat xv6_stat           // avoid clash with host struct stat
#define sleep xv6_sleep
#include "param.h"
#include "fs.h"
#include "usrbins.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

extern int clock_gettime (clockid_t, struct timespec *);

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]
// [          1 |        1 |  90 |          257 |          196 |      799455 ]
int nbitmap = FSSIZE / (BSIZE * 8) + 1; // 196 = 800000 / (512 * 8) + 1
int ninodeblocks = NINODE / IPB + 1;    // 257 = 1024 / 4 + 1
int nlog = LOGSIZE;                     // 90
int nmeta;                      // 545 = メタブロック数 (boot, sb, nlog, inode, bitmap)
int nblocks;                    // 799455 = データブロック数

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;

void balloc(int);
void wsect(uint, void *);
void winode(uint, struct dinode *);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type, uid_t uid, gid_t gid, mode_t mode);
void iappend(uint inum, void *p, int n);
uint make_dir(uint parent, char *name, uid_t uid, gid_t gid, mode_t mode);
uint make_dev(uint parent, char *name, int major, int minor, uid_t uid, gid_t gid, mode_t mode);
uint make_file(uint parent, char *name, uid_t uid, gid_t gid, mode_t mode);
void make_dirent(uint inum, uint parent, char *name);
void copy_file(int start, int argc, char *files[], uint parent, uid_t uid, gid_t gid, mode_t mode);

// convert to little-endian byte order
ushort
xshort(ushort x)
{
    ushort y;
    uchar *a = (uchar *) & y;
    a[0] = x;
    a[1] = x >> 8;
    return y;
}

uint
xint(uint x)
{
    uint y;
    uchar *a = (uchar *) & y;
    a[0] = x;
    a[1] = x >> 8;
    a[2] = x >> 16;
    a[3] = x >> 24;
    return y;
}

ulong
xlong(ulong x)
{
    ulong y;
    uchar *a = (uchar*)&y;
    a[0] = x;
    a[1] = x >> 8;
    a[2] = x >> 16;
    a[3] = x >> 24;
    a[4] = x >> 32;
    a[5] = x >> 40;
    a[6] = x >> 48;
    a[7] = x >> 56;
    return y;
}


int
main(int argc, char *argv[])
{
    int i;
    uint off;
    uint rootino, devino, sdino, binino, etcino, libino, homeino, usrino, usrbinino, localino, localbinino;
    struct dirent de;
    char buf[BSIZE];
    struct dinode din;

    static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

    if (argc < 2) {
        fprintf(stderr, "Usage: mkfs [-noroot] fs.img files...\n");
        exit(1);
    }

    //printf("BSIZE=%d, dirent=%d\n", BSIZE, sizeof(struct dirent));
    assert((BSIZE % sizeof(struct dinode)) == 0);
    assert((BSIZE % sizeof(struct dirent)) == 0);

    fsfd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fsfd < 0) {
        perror(argv[1]);
        exit(1);
    }

    // 1 fs block = 1 disk sector
    nmeta = 2 + nlog + ninodeblocks + nbitmap;
    nblocks = FSSIZE - nmeta;

    sb.size = xint(FSSIZE);
    sb.nblocks = xint(nblocks);
    sb.ninodes = xint(NINODE);
    sb.nlog = xint(nlog);
    sb.logstart = xint(2);
    sb.inodestart = xint(2 + nlog);
    sb.bmapstart = xint(2 + nlog + ninodeblocks);

    printf
        ("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
         nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

    freeblock = nmeta;          // the first free block that we can allocate

    for (i = 0; i < FSSIZE; i++)
        wsect(i, zeroes);

    memset(buf, 0, sizeof(buf));
    memmove(buf, &sb, sizeof(sb));
    wsect(1, buf);

    rootino = ialloc(T_DIR, 0, 0, S_IFDIR|0775);
    assert(rootino == ROOTINO);
    make_dirent(rootino, rootino, ".");
    make_dirent(rootino, rootino, "..");

    // Create /bin
    binino = make_dir(rootino, "bin", 0, 0, S_IFDIR|0775);

    // Create /dev
    devino = make_dir(rootino, "dev", 0, 0, S_IFDIR|0775);
    // Create /dev/sdc1
    make_dev(devino, "sdc1", SDMAJOR, 0, 0, 0, S_IFBLK|0666);
    // Create /dev/sdc2
    make_dev(devino, "sdc2", SDMAJOR, 1, 0, 0, S_IFBLK|0666);
    // Create /dev/sdc3
    make_dev(devino, "sdc3", SDMAJOR, 2, 0, 0, S_IFBLK|0666);

    // Create /dev/tty
    make_dev(devino, "tty", CONMAJOR, 0, 0, 0, S_IFCHR|0666);

    // Create /etc
    etcino = make_dir(rootino, "etc", 0, 0, S_IFDIR|0775);

    // create /lib
    libino = make_dir(rootino, "lib", 0, 0, S_IFDIR|0777);

    // Create /home
    homeino = make_dir(rootino, "home", 0, 0, S_IFDIR|0775);
    // Create /home/zuki
    make_dir(homeino, "zuki", 1000, 1000, S_IFDIR|0775);

    // create /usr
    usrino = make_dir(rootino, "usr", 0, 0, S_IFDIR|0775);
    // create /usr/bin
    usrbinino = make_dir(usrino, "bin", 0, 0, S_IFDIR|0775);
    // create /usr/local
    localino = make_dir(usrino, "local", 0, 0, S_IFDIR|0775);
    // create /usr/local/bin
    localbinino = make_dir(localino, "bin", 0, 0, S_IFDIR|0775);

    copy_file(2, argc, argv, binino, 0, 0, S_IFREG|0755);

    // /usr/bin  (coreutils)
    copy_file(0, nbins(), usrbins, usrbinino, 0, 0, S_IFREG|0755);

    // fix size of root inode dir
    rinode(rootino, &din);
    off = xint(din.size);
    off = (((off - 1) / BSIZE) + 1) * BSIZE;
    din.size = xint(off);
    winode(rootino, &din);

    balloc(freeblock);

    exit(0);
}

void
make_dirent(uint inum, uint parent, char *name)
{
    struct dirent de;

    bzero(&de, sizeof(de));
    de.inum = xint(inum);
    strncpy(de.name, name, DIRSIZ);
    //printf("DIRENT: inum=%d, name='%s' to PARNET[%d]\n", de.inum, de.name, parent);
    iappend(parent, &de, sizeof(de));
}

uint
make_dir(uint parent, char *name, uid_t uid, gid_t gid, mode_t mode)
{
    // Create parent/name
    uint inum = ialloc(T_DIR, uid, gid, mode);
    make_dirent(inum, parent, name);

    // Create parent/name/.
    make_dirent(inum, inum, ".");

    // Create parent/name/..
    make_dirent(parent, inum, "..");

    return inum;
}

uint
make_dev(uint parent, char *name, int major, int minor, uid_t uid, gid_t gid, mode_t mode)
{
    struct dinode din;

    uint inum = ialloc(T_DEV, uid, gid, mode);
    make_dirent(inum, parent, name);
    rinode(inum, &din);
    din.major = xshort(major);
    din.minor = xshort(minor);
    winode(inum, &din);
    return inum;
}

uint
make_file(uint parent, char *name, uid_t uid, gid_t gid, mode_t mode)
{
    uint inum = ialloc(T_FILE, uid, gid, mode);
    make_dirent(inum, parent, name);
    return inum;
}

void
copy_file(int start, int argc, char *files[], uint parent, uid_t uid, gid_t gid, mode_t mode)
{
    int fd, cc;
    uint inum;
    char buf[BSIZE];

    for (int i = start; i < argc; i++) {
        char *path = files[i];
        int j = 0;
        for (; *files[i]; files[i]++) {
            if (*files[i] == '/') j = -1;
            j++;
        }
        files[i] -= j;
        printf("input: '%s' -> '%s'\n", path, files[i]);

        assert(index(files[i], '/') == 0);

        if ((fd = open(path, 0)) < 0) {
            perror(files[i]);
            exit(1);
        }

        inum = make_file(parent, files[i], uid, gid, mode);
        while ((cc = read(fd, buf, sizeof(buf))) > 0)
            iappend(inum, buf, cc);
        close(fd);
    }
}

void
wsect(uint sec, void *buf)
{
    if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
        perror("lseek");
        exit(1);
    }
    if (write(fsfd, buf, BSIZE) != BSIZE) {
        perror("write");
        exit(1);
    }
}

void
winode(uint inum, struct dinode *ip)
{
    char buf[BSIZE];
    uint bn;
    struct dinode *dip;

    bn = IBLOCK(inum, sb);
    rsect(bn, buf);
    dip = ((struct dinode *)buf) + (inum % IPB);
    *dip = *ip;
    wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
    char buf[BSIZE];
    uint bn;
    struct dinode *dip;

    bn = IBLOCK(inum, sb);
    rsect(bn, buf);
    dip = ((struct dinode *)buf) + (inum % IPB);
    *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
    if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
        perror("lseek");
        exit(1);
    }
    if (read(fsfd, buf, BSIZE) != BSIZE) {
        perror("read");
        exit(1);
    }
}

uint
ialloc(ushort type, uid_t uid, gid_t gid, mode_t mode)
{
    uint inum = freeinode++;
    struct dinode din;
    struct timespec ts1, ts2;

    clock_gettime(CLOCK_REALTIME, &ts1);
    ts2.tv_sec = xlong(ts1.tv_sec);
    ts2.tv_nsec = xlong(ts1.tv_nsec);
    //printf("inum[%d] ts: sec %ld, nsec %ld\n", inum, ts2.tv_sec, ts2.tv_nsec);
    bzero(&din, sizeof(din));
    din.type  = xshort(type);
    din.nlink = xshort(1);
    din.size  = xint(0);
    din.mode  = xint(mode);
    din.uid   = xint(uid);
    din.gid   = xint(gid);
    din.atime = din.mtime = din.ctime = ts2;
    winode(inum, &din);
    return inum;
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
balloc(int used)
{
    uchar buf[BSIZE];
    int i, j, k;

    //printf("balloc: first %d blocks have been allocated\n", used);
    assert(used < nbitmap * BSIZE * 8);

    int used_blk = (used - 1) / (BSIZE * 8) + 1;

    for (j = 0; j < used_blk; j++) {
        bzero(buf, BSIZE);
        k = min(BSIZE * 8, used - j * BSIZE * 8);
        for (i = 0; i < k; i++) {
            buf[i/8] = buf[i/8] | (0x1 << (i % 8));
        }
        wsect(sb.bmapstart + j, buf);
    }
    //printf("balloc: write bitmap %d block from sector %d \n", used_blk, sb.bmapstart);
}

void
iappend(uint inum, void *xp, int n)
{
    char *p = (char *)xp;
    uint fbn, off, n1;
    struct dinode din;
    char buf[BSIZE];
    uint indirect[NINDIRECT];
    uint indirect2[NINDIRECT];
    uint x, idx1, idx2;

    rinode(inum, &din);
    off = xint(din.size);
    while (n > 0) {
        fbn = off / BSIZE;
        assert(fbn < MAXFILE);
        if (fbn < NDIRECT) {
            if (xint(din.addrs[fbn]) == 0) {
                din.addrs[fbn] = xint(freeblock++);
            }
            x = xint(din.addrs[fbn]);
        } else if (fbn < (NDIRECT + NINDIRECT)) {
            if (xint(din.addrs[NDIRECT]) == 0) {
                din.addrs[NDIRECT] = xint(freeblock++);
            }
            rsect(xint(din.addrs[NDIRECT]), (char *)indirect);
            idx1 = fbn - NDIRECT;
            if (indirect[idx1] == 0) {
                indirect[idx1] = xint(freeblock++);
                wsect(xint(din.addrs[NDIRECT]), (char *)indirect);
            }
            x = xint(indirect[idx1]);
        } else if (fbn < (NDIRECT + NINDIRECT + NINDIRECT * NINDIRECT)) {
            if (xint(din.addrs[NDIRECT + 1]) == 0) {
                din.addrs[NDIRECT + 1] = xint(freeblock++);
            }
            rsect(xint(din.addrs[NDIRECT + 1]), (char *)indirect);
            idx1 = (fbn - NDIRECT - NINDIRECT) / NINDIRECT;
            idx2 = (fbn - NDIRECT - NINDIRECT) % NINDIRECT;
            if (xint(indirect[idx1]) == 0) {
                indirect[idx1] = xint(freeblock++);
                wsect(xint(din.addrs[NDIRECT+1]), (char *)indirect);
            }
            rsect(xint(indirect[idx1]), (char *)indirect2);
            if (indirect2[idx2] == 0) {
                indirect2[idx2] = xint(freeblock++);
                wsect(xint(indirect[idx1]), (char *)indirect2);
            }
            x = xint(indirect2[idx2]);
        } else {
            printf("file is too big: fbr=%d\n", fbn);
            exit(1);
        }
        n1 = min(n, (fbn + 1) * BSIZE - off);
        rsect(x, buf);
        //if (inum == 1 && off > 0)
        //    printf("read  buf[%d]: %08x name: %s\n", off - 64, *(int *)(buf + off), buf + off + 4);
        bcopy(p, buf + off - (fbn * BSIZE), n1);
        wsect(x, buf);
        //if (inum == 1)
        //    printf("write buf[%d]: %08x name: %s\n", off, *(int *)(buf + off), buf + off + 4);
        n -= n1;
        off += n1;
        p += n1;
    }
    din.size = xint(off);
    winode(inum, &din);
}
