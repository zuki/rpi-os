#ifndef INC_VFSMOUNT_H
#define INC_VFSMOUNT_H

#define MOUNT_FREE  0
#define MOUNT_USED  1
#define MOUNTSIZE   2       // size of mounted devices

// Mount table entry
struct mntentry {
    struct inode *m_inode;
    struct inode *m_rtinode;    // Root inode for device
    void   *pdata;              // Private data of mountentry. Almost is a supberblock
    int     dev;                // Mounted device
    int     flag;               // MOUNT_FREE/MOUNT_USED
};

// Mount table structure
struct mtable {
    struct spinlock lock;
    struct mntentry mpoint[MOUNTSIZE];
};

extern struct mtable mtable;

// Utiltity functions
struct inode *mtablertinode(struct inode *ip);
struct inode *mtablemntinode(struct inode *ip);
int isinoderoot(struct inode *ip);
void mountinit(void);

#endif
