#include "types.h"
#include "console.h"
#include "spinlock.h"
#include "vfs.h"
#include "file.h"
#include "vfsmount.h"

struct mtable mtable;

// This function returns the root inode for the mount on inode
struct inode *
mtablertinode(struct inode *ip)
{
    struct inode *rtinode;
    struct mntentry *mp;

    acquire(&mtable.lock);
    for (mp = mtable.mpoint; mp < &mtable.mpoint[MOUNTSIZE]; mp++) {
        if (mp->m_inode->dev == ip->dev && mp->m_inode->inum == ip->inum) {
            rtinode = mp->m_rtinode;

            release(&mtable.lock);
            return rtinode;
        }
    }
    release(&mtable.lock);
    return 0;
}

// This function returns the mounted on inode for the root inode
struct inode *
mtablemntinode(struct inode *ip)
{
    struct inode *mntinode;
    struct mntentry *mp;

    acquire(&mtable.lock);
    for (mp = mtable.mpoint; mp < &mtable.mpoint[MOUNTSIZE]; mp++) {
        if (mp->m_rtinode->dev == ip->dev && mp->m_rtinode->inum == ip->inum) {
            mntinode = mp->m_inode;
            release(&mtable.lock);

            return mntinode;
        }
    }
    release(&mtable.lock);
    return 0;
}

int
isinoderoot(struct inode *ip)
{
    struct mntentry *mp;

    acquire(&mtable.lock);
    for (mp = mtable.mpoint; mp < &mtable.mpoint[MOUNTSIZE]; mp++) {
        if (mp->m_rtinode->dev == ip->dev && mp->m_rtinode->inum == ip->inum) {
            release(&mtable.lock);
            return 1;
        }
    }
    release(&mtable.lock);

    return 0;
}

void
mountinit(void)
{
    initlock(&mtable.lock, "mtable");
    cprintf("mountinit ok \n");
}
