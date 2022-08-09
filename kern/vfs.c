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

#include "vfs.h"
#include "types.h"
#include "buf.h"
#include "clock.h"
#include "console.h"
#include "dev.h"
#include "file.h"
#include "kmalloc.h"
#include "list.h"
#include "log.h"
#include "mmu.h"
#include "proc.h"
#include "rtc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "string.h"
#include "v6.h"
#include "ext2.h"
#include "vfsmount.h"
#include "linux/errno.h"
#include "linux/capability.h"
#include "linux/stat.h"
#include "linux/time.h"

struct vfs *rootfs;
struct vfsmlist vfsmlist;
struct icache icache;
struct superblock sb[NMINOR];

void
fs_init(void)
{
    if (init_v6fs() != 0)
        panic("cant register v6");
    if (init_ext2fs() != 0)
        panic("cant register ext2");
}

// Add rootvfs on the list
void
install_rootfs(void)
{
    if ((rootfs = (struct vfs *)kmalloc(sizeof(struct vfs))) == NULL)
        panic("Failed on rootfs allocation");

    rootfs->major = SDMAJOR;
    rootfs->minor = ROOTDEV;

    struct filesystem_type *fst = getfs(ROOTFSTYPE);
    if (fst == 0)
        panic("The root fs type is not supported");

    rootfs->fs_t = fst;

    acquire(&vfsmlist.lock);
    list_push_back(&(vfsmlist.fs_list), &(rootfs->fs_next));
    release(&vfsmlist.lock);
    info("install_rootfs ok");
}

void
init_vfsmlist(void)
{
    initlock(&vfsmlist.lock, "vfsmlist");
    list_init(&(vfsmlist.fs_list));
    debug("init_vfsmlist ok\n");
}

struct vfs *
getvfsentry(int major, int minor)
{
    struct vfs *vfs;

    LIST_FOREACH_ENTRY(vfs, &(vfsmlist.fs_list), fs_next) {
        if (vfs->major == major && vfs->minor == minor) {
            return vfs;
        }
    }

    return 0;
}

int
putvfsonlist(int major, int minor, struct filesystem_type *fs_t)
{
    struct vfs *nvfs;

    if ((nvfs = (struct vfs *)kmalloc(sizeof(struct vfs))) == NULL) {
        warn("not enough memory");
        return -1;
    }

    nvfs->major = major;
    nvfs->minor = minor;
    nvfs->fs_t  = fs_t;

    acquire(&vfsmlist.lock);
    list_push_back(&(vfsmlist.fs_list), &(nvfs->fs_next));
    release(&vfsmlist.lock);

    return 0;
}

int
deletefsfromlist(struct vfs *vfs)
{
    acquire(&vfsmlist.lock);
    list_drop(&(vfs->fs_next));
    release(&vfsmlist.lock);

    kmfree(vfs);
    return 0;
}

struct {
    struct spinlock lock;
    struct list_head fs_list;
} vfssw;

void
init_vfssw(void)
{
    initlock(&vfssw.lock, "vfssw");
    list_init(&(vfssw.fs_list));
    info("init_vfssw ok");
}

int
register_fs(struct filesystem_type *fs)
{
    acquire(&vfssw.lock);
    list_push_back(&(vfssw.fs_list), &fs->fs_list);
    release(&vfssw.lock);

    return 0;
}

struct filesystem_type*
getfs(const char *fs_name)
{
    struct filesystem_type *fs;

    LIST_FOREACH_ENTRY(fs, &(vfssw.fs_list), fs_list) {
        if (strcmp(fs_name, fs->name) == 0) {
            return fs;
        }
    }

    return 0;
}

void
generic_iunlock(struct inode *ip)
{
    int locked;
    if (ip == 0 || ((locked = holdingsleep(&ip->lock)) == 0) || ip->ref < 1) {
        warn("ip: %d, ref: %d, locked: %d", ip ? ip->inum : 0, ip ? ip->ref : 0, locked);
        panic("invalid ip\n");
    }
    releasesleep(&ip->lock);
}

void
generic_stati(struct inode *ip, struct stat *st)
{
    // FIXME: Support other fields in stat.
    st->st_dev     = ip->dev;
    st->st_ino     = ip->inum;
    st->st_nlink   = ip->nlink;
    st->st_uid     = ip->uid;
    st->st_gid     = ip->gid;
    st->st_size    = ip->size;
    st->st_blksize = sb[ip->dev].blocksize;
    st->st_blocks  = (ip->size / st->st_blksize) + 1;
    st->st_atime   = ip->atime;
    st->st_mtime   = ip->mtime;
    st->st_ctime   = ip->ctime;
    if (ip->type == T_DEV)
        st->st_rdev = makedev(ip->major, ip->minor);

    switch (ip->type) {
    case T_FILE:
    case T_DIR:
    case T_DEV:
    case T_MOUNT:
    case T_SYMLINK:
        st->st_mode = ip->mode;
        break;
    default:
        panic("unexpected stat type %d", ip->type);
    }
}

/*
 * Copy stat information from inode.
 * Caller must hold ip->lock.
 */
int
generic_dirlink(struct inode *dp, char *name, uint32_t inum, uint16_t type)
{
    ssize_t off;
    struct dirent de;
    struct inode *ip;

    /* Check that name is not present. */
    if ((ip = dp->iops->dirlookup(dp, name, 0)) != 0) {
        iput(ip);
        return -EEXIST;
    }

    /* Look for an empty dirent. */
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (dp->iops->readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
            panic("read error");
        if (de.inum == 0)
            break;
    }

    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    de.type = type;
    if (dp->iops->writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
        panic("write error");

    return 0;
}

/*
 * Read data from inode.
 * Caller must hold ip->lock.
 */
ssize_t
generic_readi(struct inode *ip, char *dst, size_t off, size_t n)
{
    trace("readi: ip=0x%llx, off=%d, n=%d", ip, off, n);
    size_t tot, m;
    struct buf *bp;
    size_t blksize = sb[ip->dev].blocksize;

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
        bp = ip->fs_t->ops->bread(ip->dev, ip->iops->bmap(ip, off / blksize));
        m = min(n - tot, blksize - (off % blksize));
        memmove(dst, bp->data + (off % blksize), m);
        ip->fs_t->ops->brelse(bp);
    }
    return n;
}

int
generic_permission(struct inode *ip, int mask)
{
    struct proc *p = thisproc();
    mode_t mode = ip->mode;

    if (p->fsuid == 0) return 0;

    if (p->fsuid == ip->uid)
        mode >>= 6;
    else if (p->fsgid == ip->gid)
        mode >>= 3;

    if (((mode & mask & (MAY_READ|MAY_WRITE|MAY_EXEC)) == mask))
        return 0;

    if ((mask & (MAY_READ|MAY_WRITE)) || (ip->mode & S_IXUGO))
        if (capable(CAP_DAC_OVERRIDE))
            return 0;

    return -EACCES;
}

/* Inodes.
 *
 * An inode describes a single unnamed file.
 * The inode disk structure holds metadata: the file's type,
 * its size, the number of links referring to it, and the
 * list of blocks holding the file's content.
 *
 * The inodes are laid out sequentially on disk at
 * sb.startinode. Each inode has a number, indicating its
 * position on the disk.
 *
 * The kernel keeps a cache of in-use inodes in memory
 * to provide a place for synchronizing access
 * to inodes used by multiple processes. The cached
 * inodes include book-keeping information that is
 * not stored on disk: ip->ref and ip->valid.
 *
 * An inode and its in-memory representation go through a
 * sequence of states before they can be used by the
 * rest of the file system code.
 *
 * * Allocation: an inode is allocated if its type (on disk)
 *   is non-zero. ialloc() allocates, and iput() frees if
 *   the reference and link counts have fallen to zero.
 *
 * * Referencing in cache: an entry in the inode cache
 *   is free if ip->ref is zero. Otherwise ip->ref tracks
 *   the number of in-memory pointers to the entry (open
 *   files and current directories). iget() finds or
 *   creates a cache entry and increments its ref; iput()
 *   decrements ref.
 *
 * * Valid: the information (type, size, &c) in an inode
 *   cache entry is only correct when ip->valid is 1.
 *   ilock() reads the inode from
 *   the disk and sets ip->valid, while iput() clears
 *   ip->valid if ip->ref has fallen to zero.
 *
 * * Locked: file system code may only examine and modify
 *   the information in an inode and its content if it
 *   has first locked the inode.
 *
 * Thus a typical sequence is:
 *   ip = iget(dev, inum)
 *   ilock(ip)
 *   ... examine and modify ip->xxx ...
 *   iunlock(ip)
 *   iput(ip)
 *
 * ilock() is separate from iget() so that system calls can
 * get a long-term reference to an inode (as for an open file)
 * and only lock it for short periods (e.g., in read()).
 * The separation also helps avoid deadlock and races during
 * pathname lookup. iget() increments ip->ref so that the inode
 * stays cached and pointers to it remain valid.
 *
 * Many internal file system functions expect the caller to
 * have locked the inodes involved; this lets callers create
 * multi-step atomic operations.
 *
 * The icache.lock spin-lock protects the allocation of icache
 * entries. Since ip->ref indicates whether an entry is free,
 * and ip->dev and ip->inum indicate which i-node an entry
 * holds, one must hold icache.lock while using any of those fields.
 *
 * An ip->lock sleep-lock protects all ip-> fields other than ref,
 * dev, and inum.  One must hold ip->lock in order to
 * read or write that inode's ip->valid, ip->size, ip->type, &c.
 */

void
iinit(int dev)
{
    initlock(&icache.lock, "icache");
    for (int i = 0; i < NINODE; i++) {
        initsleeplock(&icache.inode[i].lock, "inode");
    }
    rootfs->fs_t->ops->readsb(dev, &sb[dev]);
    struct v6_superblock *v6sb = (struct v6_superblock *)sb[dev].fs_info;
    info("sb: size %d nblocks %d ninodes %d nlog %d logstart %d inodestart %d bmapstart %d",
            v6sb->size, v6sb->nblocks, v6sb->ninodes, v6sb->nlog, v6sb->logstart, v6sb->inodestart, v6sb->bmapstart);
}

/*
 * Find the inode with number inum on device dev
 * and return the in-memory copy. Does not lock
 * the inode and does not read it from disk.
 */
struct inode *
iget(uint32_t dev, uint32_t inum, uint16_t type, int (*fill_inode)(struct inode *))
{
    struct inode *ip, *nip = 0;
    struct filesystem_type *fs_t;

    acquire(&icache.lock);
    for (ip = icache.inode; ip < &icache.inode[NINODE]; ip++) {
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            // If the current inode is an mount point
            if (ip->type == T_MOUNT) {
                struct inode *rinode = mtablertinode(ip);
                if (rinode == 0) panic("Invalid inode on mount table");

                rinode->ref++;
                release(&icache.lock);
                return rinode;
            }

            ip->ref++;
            release(&icache.lock);
            return ip;
        }
        if (nip == 0 && ip->ref == 0)
            nip = ip;
    }

    if (nip == 0) panic("iget: no inodes\n");

    fs_t = getvfsentry(SDMAJOR, dev)->fs_t;

    nip->dev = dev;
    nip->inum = inum;
    nip->type = type;
    nip->ref = 1;
    nip->valid = 0;
    nip->fs_t = fs_t;
    nip->iops = fs_t->iops;
    release(&icache.lock);

    if (!fill_inode(nip)) panic("iget: fill inode");

    return nip;
}

/*
 * Increment reference count for ip.
 * Returns ip to enable ip = idup(ip1) idiom.
 */
struct inode*
idup(struct inode *ip)
{
    acquire(&icache.lock);
    ip->ref++;
    release(&icache.lock);
    return ip;
}

/* Drop a reference to an in-memory inode.
 *
 * If that was the last reference, the inode cache entry can
 * be recycled.
 * If that was the last reference and the inode has no links
 * to it, free the inode (and its content) on disk.
 * All calls to iput() must be inside a transaction in
 * case it has to free the inode.
 */
void
iput(struct inode *ip)
{
    acquiresleep(&ip->lock);
    if (ip->valid && ip->nlink == 0) {
        acquire(&icache.lock);
        int r = ip->ref;
        release(&icache.lock);
        if (r==1) {
            // inode has no link and no other ref: truncate and free
            ip->iops->itrunc(ip);
            ip->type = 0;
            ip->iops->iupdate(ip);
            ip->valid = 0;
        }
    }
    releasesleep(&ip->lock);

    acquire(&icache.lock);
    ip->ref--;
    if (ip->ref == 0) {
        ip->iops->cleanup(ip);
    }
    release(&icache.lock);
}


/* Common idiom: unlock, then put. */
void
iunlockput(struct inode *ip)
{
    ip->iops->iunlock(ip);
    iput(ip);
}

/* Paths. */

/* Copy the next path element from path into name.
 *
 * Return a pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path=='\0' to see if the name is the last one.
 * If no name to remove, return 0.
 *
 * Examples:
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */
static char*
skipelem(char *path, char *name)
{
    while (*path == '/') path++;
    if (*path == 0) return 0;
    char *s = path;
    while (*path != '/' && *path != 0) path++;
    int len = path - s;
    if (len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while (*path == '/') path++;
    return path;
}

/* Look up and return the inode for a path name.
 *
 * If parent != 0, return the inode for the parent and copy the final
 * path element into name, which must have room for DIRSIZ bytes.
 * Must be called inside a transaction since it calls iput().
 */
static struct inode *
namex(char *path, int nameiparent, char *name)
{
    struct inode *ip, *next, *ir;

    if (*path == '/')
        ip = rootfs->fs_t->ops->getroot(SDMAJOR, ROOTDEV);
    else
        ip = idup(thisproc()->cwd);

    while ((path = skipelem(path, name)) != 0) {
        debug("path: '%s'", path);
        ip->iops->ilock(ip);
        if (ip->type != T_DIR) {
            iunlockput(ip);
            debug("ip '%d' is not directory", ip->inum);
            return 0;
        }
        if (nameiparent && *path == '\0') {
            // Stop one level early.
            ip->iops->iunlock(ip);
            return ip;
        }

component_search:
        if ((next = ip->iops->dirlookup(ip, name, 0)) == 0) {
            iunlockput(ip);
            debug("ip '%d', name: '%s' is not found", ip->inum, name);
            return 0;
        }
        ir = next->fs_t->ops->getroot(SDMAJOR, next->dev);
        if (next->inum == ir->inum && isinoderoot(ip) && (strncmp(name, "..", 2)) == 0) {
            struct inode *mntinode = mtablemntinode(ip);
            iunlockput(ip);
            ip = mntinode;
            ip->iops->ilock(ip);
            ip->ref++;
            goto component_search;
        }

        iunlockput(ip);
        ip = next;
    }
    if (nameiparent) {
        iput(ip);
        return 0;
    }
    return ip;
}

struct inode *
namei(char *path)
{
    char name[DIRSIZ];
    debug("path: %s", path);
    return namex(path, 0, name);
}

struct inode *
nameiparent(char *path, char *name)
{
    return namex(path, 1, name);
}

int
sb_set_blocksize(struct superblock *sb, uint32_t size)
{
    /* If we get here, we know size is power of two
     * and it's value is between 512 and PAGE_SIZE
     */
    sb->blocksize = size;
    sb->s_blocksize_bits = blksize_bits(size);
    return sb->blocksize;
}

void
print_dirent64(struct dirent64 *de) {
    cprintf("de64: d_ino: %ld, d_off: %ld, d_reclen: %d, d_type: %d, d_name: %s\n",
            de->d_ino, de->d_off, de->d_reclen, de->d_type, de->d_name);
}

struct inode *
create(char *path, uint16_t type, uint16_t major, uint16_t minor, mode_t mode)
{
    struct inode *dp, *ip;
    char name[255];         // ext2のファイル名は最大255
    size_t off;
    struct timespec ts;
    long ret;

    trace("path=%s, uid=%d, gid=%d", path, thisproc()->uid, thisproc()->gid);
    // 親ディレクトリなし
    if ((dp = nameiparent(path, name)) == 0)
        return (void *)-ENOENT;
    dp->iops->ilock(dp);
    if ((ret = dp->iops->permission(dp, MAY_EXEC)) < 0) {
        iunlockput(dp);
        return (void *)-EACCES;
    }

    // ファイルはすでにあり
    if ((ip = dp->iops->dirlookup(dp, name, &off)) != 0) {
        iunlockput(dp);
        ip->iops->ilock(ip);
        if (type == T_FILE && ip->type == T_FILE) {
            return ip;
        }
        iunlockput(ip);
        return (void *)-EEXIST;
    }

    // ファイルなし、作成
    if ((ip = dp->fs_t->ops->ialloc(dp->dev, type)) == 0)
        panic("create: no inodes\n");
    ip->iops->ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    ip->mode  = mode & ~thisproc()->umask;
    ip->type  = type;
    clock_gettime(CLOCK_REALTIME, &ts);
    ip->atime = ip->mtime = ip->ctime = ts;
    ip->uid = thisproc()->uid;
    ip->gid = thisproc()->gid;
    ip->iops->iupdate(ip);
    if (type == T_DIR) {
        dp->nlink++;
        dp->iops->iupdate(dp);
        if (ip->iops->dirlink(ip, ".", ip->inum, ip->type) < 0 || ip->iops->dirlink(ip, "..", dp->inum, dp->type) < 0)
            panic("create: cant dots\n");
    }

    if (dp->iops->dirlink(dp, name, ip->inum, ip->type) < 0)
        panic("create: dirlink\n");
    iunlockput(dp);
    return ip;
}
