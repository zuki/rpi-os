/*
 * File system implementation.  Five layers:
 *   + Blocks: allocator for raw disk blocks.
 *   + Log: crash recovery for multi-step updates.
 *   + Files: inode allocator, reading, writing, metadata.
 *   + Directories: inode with special contents (list of other inodes!)
 *   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
 *
 * This file contains the low-level file system manipulation
 * routines.  The (higher-level) system call implementations
 * are in sysfile.c.
 */
#include "types.h"
#include "vfs.h"
#include "v6.h"
#include "vfsmount.h"
#include "buf.h"
#include "clock.h"
#include "file.h"
#include "kmalloc.h"
#include "log.h"
#include "mmu.h"
#include "proc.h"
#include "string.h"
#include "console.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "linux/errno.h"
#include "linux/time.h"

// There should be one superblock per disk device,
// but we run with only one device.

struct vfs_operations v6_ops = {
    .fs_init    = &v6fs_init,
    .mount      = &v6_mount,
    .unmount    = &v6_unmount,
    .getroot    = &v6_getroot,
    .readsb     = &v6_readsb,
    .ialloc     = &v6_ialloc,
    .balloc     = &v6_balloc,
    .bzero      = &v6_bzero,
    .bfree      = &v6_bfree,
    .brelse     = &brelse,
    .bwrite     = &bwrite,
    .bread      = &bread,
    .namecmp    = &v6_namecmp,
    .direntlookup = &v6_direntlookup,
    .getrootino = &v6_getrootino,
    .getdents   = &v6_getdents
};

struct inode_operations v6_iops = {
    .dirlookup  = &v6_dirlookup,
    .iupdate    = &v6_iupdate,
    .itrunc     = &v6_itrunc,
    .cleanup    = &v6_cleanup,
    .bmap       = &v6_bmap,
    .ilock      = &v6_ilock,
    .iunlock    = &generic_iunlock,
    .stati      = &generic_stati,
    .readi      = &v6_readi,
    .writei     = &v6_writei,
    .dirlink    = &generic_dirlink,
    .unlink     = &v6_unlink,
    .isdirempty = &v6_isdirempty,
    .permission = &generic_permission
};

struct filesystem_type v6fs = {
    .name       = "v6",
    .ops        = &v6_ops,
    .iops       = &v6_iops,
};

int
init_v6fs(void)
{
    return register_fs(&v6fs);
}

int
v6_fill_inode(struct inode *ip)
{
    struct v6_inode *v6ip = (struct v6_inode *)kmalloc(sizeof(struct v6_inode));
    ip->i_private = v6ip;
    return 1;
}

/*
 * Find the inode with number inum on device dev
 * and return the in-memory copy. Does not lock
 * the inode and does not read it from disk.
 */
static struct inode *
v6_iget(uint32_t dev, uint32_t inum, uint16_t type)
{
    return iget(dev, inum, type, &v6_fill_inode);
}

/*
 * FILESYSTEMS OPERATIONS
 */

int
v6fs_init(void)
{
    return 0;
}

int
v6_mount(struct inode *devi, struct inode *ip)
{
    struct mntentry *mp;

    // Read the Superblock
    v6_readsb(devi->minor, &sb[devi->minor]);

    // Read the root device
    struct inode *devrtip = v6_getroot(devi->major, devi->minor);

    acquire(&mtable.lock);
    for (mp = mtable.mpoint; mp < &mtable.mpoint[MOUNTSIZE]; mp++) {
        // This slot is available
        if (mp->flag == MOUNT_FREE) {
found_slot:
            mp->dev       = devi->minor;
            mp->m_inode   = ip;
            mp->pdata     = &sb[devi->minor];
            mp->flag      = MOUNT_USED;
            mp->m_rtinode = devrtip;

            release(&mtable.lock);
            initlog(devi->minor);
            return 0;
        } else {
            // The disk is already mounted
            if (mp->dev == devi->minor) {
                release(&mtable.lock);
                return -1;
            }

            if (ip->dev == mp->m_inode->dev && ip->inum == mp->m_inode->inum)
                goto found_slot;
        }
    }
    release(&mtable.lock);

    return -1;
}

int
v6_unmount(struct inode *devi)
{
    struct mntentry *mp;

    acquire(&mtable.lock);
    for (mp = mtable.mpoint; mp < &mtable.mpoint[MOUNTSIZE]; mp++) {
        // found the target device
        if (mp->dev == devi->dev) {
            mp->dev = -1;
            mp->flag = MOUNT_FREE;
            release(&mtable.lock);
            return 0;
        }
    }
    release(&mtable.lock);
    return -1;
}

struct inode *
v6_getroot(int major, int minor)
{
    return v6_iget(minor, V6_ROOTINO, T_DIR);
}

/* Read the super block. */
void
v6_readsb(uint32_t dev, struct superblock *sb)
{
    struct buf *bp;
    struct v6_superblock *v6sb;

    if (sb->flags == SB_INITIALIZED) return;

    v6sb = (struct v6_superblock *)kmalloc(sizeof(struct v6_superblock));
    if (v6sb == NULL) panic("momory exhaust");

    // These sets are needed to bread
    sb->major = SDMAJOR;
    sb->minor = dev;
    sb_set_blocksize(sb, BSIZE);
    sb->lba = mbr.ptables[V6MINOR].lba;
    sb->nsecs = mbr.ptables[V6MINOR].nsecs;
    sb->flags = SB_INITIALIZED;

    bp = v6_ops.bread(dev, 1);
    memmove(v6sb, bp->data, sizeof(*v6sb));
    v6_ops.brelse(bp);
    sb->fs_info = v6sb;
}

/* Allocate an inode on device dev.
 *
 * Mark it as allocated by giving it type type.
 * Returns an unlocked but allocated and referenced inode.
 */
struct inode *
v6_ialloc(uint32_t dev, uint16_t type)
{
    int inum;
    struct buf *bp;
    struct dinode *dip;
    struct v6_superblock *v6sb = sb[dev].fs_info;

    for (inum = 1; inum < v6sb->ninodes; inum++) {
        bp = v6_ops.bread(dev, IBLOCK(inum, (*v6sb)));
        dip = (struct dinode *)bp->data + inum % IPB;
        if (dip->type == 0) {   // a free inode
            memset(dip, 0, sizeof(*dip));
            dip->type = type;
            log_write(bp);      // mark it allocated on the disk
            v6_ops.brelse(bp);
            struct inode *ip = v6_iget(dev, inum, type);
            struct timespec tp;
            clock_gettime(CLOCK_REALTIME, &tp);
            ip->atime = ip->ctime = ip->mtime = tp;
            return ip;
        }
        v6_ops.brelse(bp);
    }
    panic("ialloc: no inodes");
    return 0;
}

/* Allocate a zeroed disk block. */
uint32_t
v6_balloc(uint32_t dev)
{
    int b, bi, m;
    struct buf *bp;
    struct v6_superblock *v6sb = sb[dev].fs_info;

    bp = 0;
    for (b = 0; b < v6sb->ninodes; b += BPB) {
        bp = v6_ops.bread(dev, BBLOCK(b, (*v6sb)));
        for (bi = 0; bi < BPB && b + bi < v6sb->size; bi++) {
            m = 1 << (bi % 8);
            if ((bp->data[bi / 8] & m) == 0) {  // Is block free?
                bp->data[bi / 8] |= m;  // Mark block in use.
                log_write(bp);
                v6_ops.brelse(bp);
                v6_bzero(dev, b + bi);
                return b + bi;
            }
        }
        v6_ops.brelse(bp);
    }
    panic("balloc: out of blocks");
    return 0;
}

/* Zero a block. */
void
v6_bzero(uint32_t dev, int bno)
{
    struct buf *bp;

    bp = v6_ops.bread(dev, bno);
    memset(bp->data, 0, BSIZE);
    log_write(bp);
    v6_ops.brelse(bp);
}

/* Free a disk block. */
void
v6_bfree(uint32_t dev, uint32_t b)
{
    struct buf *bp;
    int bi, m;
    struct v6_superblock *v6sb;

    v6_readsb(dev, &sb[dev]);
    v6sb = sb[dev].fs_info;

    bp = v6_ops.bread(dev, BBLOCK(b, (*v6sb)));
    bi = b % BPB;
    m = 1 << (bi % 8);
    if ((bp->data[bi / 8] & m) == 0)
        panic("freeing free block");
    bp->data[bi / 8] &= ~m;
    log_write(bp);
    v6_ops.brelse(bp);
}

/* Directories. */

int
v6_namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

/* 指定のディレクトリから指定のinumを持つディレクトリエントリを検索する */
int
v6_direntlookup(struct inode *dp, int inum, struct dirent *dep, size_t *ofp)
{
    struct dirent de;

    if (dp->type != T_DIR) panic("dp is not DIR");

    for (uint32_t off = 0; off < dp->size; off += sizeof(de)) {
        if (v6_readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
            panic("readi");
        if (de.inum == inum) {
            if (ofp) *ofp = off;
            memmove(dep, &de, sizeof(de));
            return 0;
        }
    }
    return -1;
}

uint16_t
v6_getrootino(void)
{
    return V6_ROOTINO;
}

/*
 * INODE OPERATIONS
 */

/*
 * Look for a directory entry in a directory.
 * If found, set *poff to byte offset of entry.
 * If not found, set *poff to byte offset of 1st free entry.
 */
struct inode *
v6_dirlookup(struct inode *dp, char *name, size_t *poff)
{
    size_t off;
    ssize_t free = -1;
    struct dirent de;

    if (dp->type == T_MOUNT) {
        struct inode *rinode = mtablertinode(dp);
        if (rinode == 0) panic("v6_dirlookup: Invalid inode on mount table\n");
        rinode->ref++;
        return rinode;
    }

    if (dp->type != T_DIR)
        panic("dirlookup not DIR");

    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (v6_readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlookup read");
        if (de.inum == 0) {
            if (free == -1) free = off;
            continue;
        }
        if (v6_namecmp(name, de.name) == 0) {
            // entry matches path element
            if (poff)
                *poff = off;
            return v6_iget(dp->dev, de.inum, de.type);
        }
    }
    if (poff)
        *poff = (size_t)free;
    return 0;
}

/* Copy a modified in-memory inode to disk.
 *
 * Must be called after every change to an ip->xxx field
 * that lives on disk, since i-node cache is write-through.
 * Caller must hold ip->lock.
 */
void
v6_iupdate(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;
    struct v6_inode *v6ip = ip->i_private;
    struct v6_superblock *v6sb = sb[ip->dev].fs_info;

    bp = v6_ops.bread(ip->dev, IBLOCK(ip->inum, (*v6sb)));
    dip = (struct dinode *)bp->data + ip->inum % IPB;
    dip->type  = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size  = ip->size;
    dip->uid   = ip->uid;
    dip->gid   = ip->gid;
    dip->mode  = ip->mode;
    dip->atime = ip->atime;
    dip->mtime = ip->mtime;
    dip->ctime = ip->ctime;
    memmove(dip->addrs, v6ip->addrs, sizeof(v6ip->addrs));
    log_write(bp);
    v6_ops.brelse(bp);
}

/* Truncate inode (discard contents).
 *
 * Only called when the inode has no links
 * to it (no directory entries referring to it)
 * and has no in-memory reference to it (is
 * not an open file or current directory).
 */
void
v6_itrunc(struct inode *ip)
{
    struct buf *bp;
    uint32_t *a, *b;
    struct v6_inode *v6ip = ip->i_private;

    for (int i = 0; i < NDIRECT; i++) {
        if (v6ip->addrs[i]) {
            v6_bfree(ip->dev, v6ip->addrs[i]);
            v6ip->addrs[i] = 0;
        }
    }

    if (v6ip->addrs[NDIRECT]) {
        bp = v6_ops.bread(ip->dev, v6ip->addrs[NDIRECT]);
        a = (uint32_t *) bp->data;
        for (int j = 0; j < NINDIRECT; j++) {
            if (a[j]) {
                v6_bfree(ip->dev, a[j]);
                a[j] = 0;
            }
        }
        v6_ops.brelse(bp);
        v6_bfree(ip->dev, v6ip->addrs[NDIRECT]);
        v6ip->addrs[NDIRECT] = 0;
    }

    if (v6ip->addrs[NDIRECT+1]) {
        bp = v6_ops.bread(ip->dev, v6ip->addrs[NDIRECT+1]);
        a = (uint32_t *)bp->data;
        for (int i = 0; i < NINDIRECT; i++) {
            if (a[i]) {
                v6_ops.brelse(bp);
                bp = v6_ops.bread(ip->dev, a[i]);
                b = (uint32_t *)bp->data;
                for (int j = 0; j < NINDIRECT; j++) {
                    if (b[j]) {
                        v6_bfree(ip->dev, b[j]);
                        b[j] = 0;
                    }
                }
                v6_bfree(ip->dev, a[i]);
                a[i] = 0;
            }
        }
        v6_ops.brelse(bp);
        v6_bfree(ip->dev, v6ip->addrs[NDIRECT+1]);
        v6ip->addrs[NDIRECT+1] = 0;
    }

    ip->size = 0;
    v6_iupdate(ip);
}

void
v6_cleanup(struct inode *ip)
{
    memset(ip->i_private, 0, sizeof(struct v6_inode));
    kmfree(ip->i_private);
}

/* Inode content
 *
 * The content (data) associated with each inode is stored
 * in blocks on the disk. The first NDIRECT block numbers
 * are listed in ip->addrs[].  The next NINDIRECT blocks are
 * listed in block ip->addrs[NDIRECT].
 *
 * Return the disk block address of the nth block in inode ip.
 * If there is no such block, bmap allocates one.
 */
uint32_t
v6_bmap(struct inode *ip, uint32_t bn)
{
    uint32_t idx1, idx2, addr, *a;
    struct buf *bp;
    struct v6_inode *v6ip = (struct v6_inode *)ip->i_private;

    if (bn < NDIRECT) {
        if ((addr = v6ip->addrs[bn]) == 0)
            v6ip->addrs[bn] = addr = v6_balloc(ip->dev);
        return addr;
    }
    bn -= NDIRECT;

    if (bn < NINDIRECT) {
        // Load indirect block, allocating if necessary.
        if ((addr = v6ip->addrs[NDIRECT]) == 0)
            v6ip->addrs[NDIRECT] = addr = v6_balloc(ip->dev);
        bp = v6_ops.bread(ip->dev, addr);
        a = (uint32_t *) bp->data;
        if ((addr = a[bn]) == 0) {
            a[bn] = addr = v6_balloc(ip->dev);
            log_write(bp);
        }
        v6_ops.brelse(bp);
        return addr;
    }

    bn -= NINDIRECT;

    if (bn < NINDIRECT * NINDIRECT) {
        // 二重間接ブロックをロードする。必要であれば割り当てる。
        if ((addr = v6ip->addrs[NDIRECT+1]) == 0)
            v6ip->addrs[NDIRECT+1] = addr = v6_balloc(ip->dev);
        idx1 = bn / NINDIRECT;  // ip->addrs[NDIRECT+1]内のインデックス
        idx2 = bn % NINDIRECT;  // ip->addrs[NDIRECT+1][idex1]内のインデックス
        bp = v6_ops.bread(ip->dev, addr);
        a = (uint32_t *)bp->data;
        if ((addr = a[idx1]) == 0) {  // 二重間接の1段階目
            a[idx1] = addr = v6_balloc(ip->dev);
            log_write(bp);
        }
        v6_ops.brelse(bp);
        bp = v6_ops.bread(ip->dev, addr);
        a = (uint32_t *)bp->data;
        if ((addr = a[idx2]) == 0) {  // 二重間接の2段階目
            a[idx2] = addr = v6_balloc(ip->dev);
            log_write(bp);
        }
        v6_ops.brelse(bp);
        return addr;
    }

    panic("bmap: out of range");
    return 0;
}

/*
 * Lock the given inode.
 * Reads the inode from disk if necessary.
 */
void
v6_ilock(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;

    if (ip == 0 || ip->ref < 1)
        panic("ilock");

    struct v6_superblock *v6sb = sb[ip->dev].fs_info;
    struct v6_inode *v6ip = (struct v6_inode *)ip->i_private;

    acquiresleep(&ip->lock);
    if (ip->valid == 0) {
        bp = v6_ops.bread(ip->dev, IBLOCK(ip->inum, (*v6sb)));
        dip = (struct dinode *)bp->data + ip->inum % IPB;
        ip->type = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->nlink = dip->nlink;
        ip->size = dip->size;
        ip->mode  = dip->mode;
        ip->uid   = dip->uid;
        ip->gid   = dip->gid;
        ip->atime = dip->atime;
        ip->mtime = dip->mtime;
        ip->ctime = dip->ctime;
        memmove(v6ip->addrs, dip->addrs, sizeof(dip->addrs));
        v6_ops.brelse(bp);
        ip->valid = 1;
        if (ip->type == 0)
            panic("ilock: no type");
    }
}

/*
 * Read data from inode.
 * Caller must hold ip->lock.
 */
ssize_t
v6_readi(struct inode *ip, char *dst, size_t off, size_t n)
{
    size_t tot, m;
    struct buf *bp;

    if (ip->type == T_DEV) {
        if (ip->major < 0 || ip->major >= NMAJOR || !devsw[ip->major].read)
            return -1;
        return devsw[ip->major].read(ip, dst, n);
    }

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > ip->size)
        n = ip->size - off;
    for (tot = 0; tot < n; tot += m, off += m, dst += m) {
        debug("dev: %d, off: 0x%llx, blkno: 0x%x", ip->dev, off, v6_bmap(ip, off / BSIZE));
        bp = v6_ops.bread(ip->dev, v6_bmap(ip, off / BSIZE));
        m = min(n - tot, BSIZE - off % BSIZE);
        memmove(dst, bp->data + off % BSIZE, m);
        v6_ops.brelse(bp);
    }

    return n;
}

/*
 * Write data to inode.
 * Caller must hold ip->lock.
 */
ssize_t
v6_writei(struct inode *ip, char *src, size_t off, size_t n)
{
    size_t tot, m;
    struct buf *bp;

    if (ip->type == T_DEV) {
        if (ip->major < 0 || ip->major >= NMAJOR || !devsw[ip->major].write)
            return -1;
        return devsw[ip->major].write(ip, src, n);
    }

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE * BSIZE)
        return -1;

    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        bp = v6_ops.bread(ip->dev, v6_bmap(ip, off / BSIZE));
        m = min(n - tot, BSIZE - off % BSIZE);
        memmove(bp->data + off % BSIZE, src, m);
        log_write(bp);
        v6_ops.brelse(bp);
    }

    if (n > 0 && off > ip->size) {
        ip->size = off;
        v6_iupdate(ip);
    }
    return n;
}

int
v6_unlink(struct inode *dp, uint32_t off)
{
    struct dirent de;
    // FIXME: 取り詰めとsizeの変更
    memset(&de, 0, sizeof(de));
    if (dp->iops->writei(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
        return -1;
    return 0;
}

// Is the directory dp empty except for "." and ".."
int
v6_isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
        if (v6_readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
            panic("readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}

/*
 * V5 SPECIFIC functions
 */
long
v6_getdents(struct file *f, char *data, size_t size)
{
    ssize_t r, n;
    int namelen, reclen, off = 0;
    char *buf;
    struct dirent de;
    struct dirent64 *de64;

    buf = data;

    while (1) {
        n = fileread(f, (char *)&de, sizeof(de));
        r = (buf - data);
        if (n == 0) {
            trace("read 0");
            return r ? r : 0;
        }
        if (n < 0 || n != sizeof(de)) {
            trace("readi failed");
            return r ? r : -1;
        }

        if (de.inum == 0) continue;

        namelen = MIN(strlen(de.name), DIRSIZ) + 1;
        reclen = (size_t)(&((struct dirent64*)0)->d_name);
        reclen = reclen + namelen;
        reclen = ALIGN(reclen, 3);
        if ((r + reclen) > size) {
            trace("break; r: %d, reclen: %d, size: %d", r, reclen, size);
            break;
        }

        de64 = (struct dirent64 *)buf;
        memset(de64, 0, sizeof(struct dirent64));
        de64->d_ino = de.inum;
        de64->d_off = off;
        de64->d_reclen = reclen;
        de64->d_type = IFTODT(f->ip->mode);
        strncpy(de64->d_name, de.name, namelen);
        buf += reclen;
        off = f->off;
    }

    return (buf - data);
}
