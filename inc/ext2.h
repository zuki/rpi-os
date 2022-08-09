#ifndef INC_EXT2_H
#define INC_EXT2_H

#include "types.h"
#include "console.h"
#include "vfs.h"
#include "linux/stat.h"

// data type for block ofset of block group
typedef int             ext2_grpblk_t;

// data type for filesystem-wide blocks number
typedef unsigned long   ext2_fsblk_t;

#define EXT2_MIN_BLKSIZE    1024
#define EXT2_SUPER_MAGIC    0xEF53
#define EXT2_MAX_BGC        40
#define EXT2_NAME_LEN       255
#define EXT2_NINODE         1024
/**
 * EXT2_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define EXT2_DIR_PAD                4
#define EXT2_DIR_ROUND              (EXT2_DIR_PAD - 1)
// 8 = sizeof(struct ext2_dir_entry_2) - name[], 4バイト境界に合わせる
#define EXT2_DIR_REC_LEN(name_len)  (((name_len) + 8 + EXT2_DIR_ROUND) & \
                                     ~EXT2_DIR_ROUND)
#define EXT2_MAX_REC_LEN            ((1<<16) - 1)

/**
 * Ext2 directory file types. Only the low 3 bits are used.
 * The other bits are reserved for now.
 */
enum {
    EXT2_FT_UNKNOWN     = 0,
    EXT2_FT_REG_FILE    = 1,
    EXT2_FT_DIR         = 2,
    EXT2_FT_CHRDEV      = 3,
    EXT2_FT_BLKDEV      = 4,
    EXT2_FT_FIFO        = 5,
    EXT2_FT_SOCK        = 6,
    EXT2_FT_SYMLINK     = 7,
    EXT2_FT_MAX
};

/**
 * ext2 super-block data in memory
 */
struct ext2_sb_info {
    unsigned long s_inodes_per_block;   // Number of inodes per block
    unsigned long s_blocks_per_group;   // Number of blocks in a group
    unsigned long s_inodes_per_group;   // Number of inodes in a group
    unsigned long s_itb_per_group;      // Number of inode table blocks per group
    unsigned long s_gdb_count;          // Number of group descriptor blocks
    unsigned long s_desc_per_block;     // Number of group descriptors per block
    unsigned long s_groups_count;       // Number of groups in the fs
    unsigned long s_overhead_last;      // Last calculated overhead
    unsigned long s_blocks_last;        // Last seen block count
    struct buf   *s_sbh;                // Buffer containing the super block
    struct ext2_superblock *s_es;       // Pointer to the super block in the buffer
    struct buf   *s_group_desc[EXT2_MAX_BGC];
    unsigned long s_sb_block;
    unsigned short s_pad;
    int           s_addr_per_block_bits;
    int           s_desc_per_block_bits;
    int           s_inode_size;
    int           s_first_ino;
    unsigned long s_dir_count;
    uint8_t      *s_debts;
    int           flags;
};

static inline struct ext2_sb_info *
EXT2_SB(struct superblock *sb)
{
    return sb->fs_info;
}

/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT2_MIN_BLOCK_SIZE         1024
#define EXT2_MAX_BLOCK_SIZE         4096
#define EXT2_BLOCK_SIZE(s)          ((s)->blocksize)
#define EXT2_ADDR_PER_BLOCK(s)      (EXT2_BLOCK_SIZE(s) / sizeof(uint32_t))
#define EXT2_BLOCK_SIZE_BITS(s)     ((s)->s_blocksize_bits)
#define EXT2_ADDR_PER_BLOCK_BITS(s) (EXT2_SB(s)->s_addr_per_block_bits)
#define EXT2_INODE_SIZE(s)          (EXT2_SB(s)->s_inode_size)
#define EXT2_FIRST_INO(s)           (EXT2_SB(s)->s_first_ino)

/**
 * This strcut is based on the Linux Source Code fs/ext2/ext2.h
 * It is the ext2 superblock layout definition.
 */
struct ext2_superblock {
    uint32_t s_inodes_count;        // inodes count
    uint32_t s_blocks_count;        // blocks count
    uint32_t s_r_blocks_count;      // reserved blocks count
    uint32_t s_free_blocks_count;   // free blocks count
    uint32_t s_free_inodes_count;   // free inodes count
    uint32_t s_first_data_block;    // first data block
    uint32_t s_log_block_size;      // block size
    uint32_t s_log_frag_size;       // fragment size;
    uint32_t s_blocks_per_group;    // blocks per group
    uint32_t s_frags_per_group;     // fragments per group
    uint32_t s_indoes_per_group;    // inodes per group
    uint32_t s_mtime;               // mount time
    uint32_t s_wtime;               // write time
    uint16_t s_mnt_count;           // mount count
    uint16_t s_max_mnt_count;       // maximal mount count
    uint16_t s_magic;               // magic signature
    uint16_t s_state;               // file system state
    uint16_t s_errors;              // behaviour when detecting errors
    uint16_t s_minor_rev_level;     // minor revision level
    uint32_t s_lastcheck;           // time of last check
    uint32_t s_checkinterval;       // max. time between checks
    uint32_t s_creator_os;          // OS
    uint32_t s_rev_level;           // revision level
    uint16_t s_def_resuid;          // default uid for reserved blocks
    uint16_t s_def_resgid;          // default gid for reserved blocks

    /**
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * NOTE: the difference between the compatible feature set and
     * the incomptible feature set is taht if here is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requiremnets are more strict; if it doesn' know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it dowsn't understand...
     */
    uint32_t s_first_ino;           // first non-reserved inode
    uint16_t s_inode_size;          // size of inode structure
    uint16_t s_block_group_nr;      // block group # of this superblock
    uint32_t s_feature_compat;      // compatible feature set
    uint32_t s_feature_incompat;    // cincompatible feature set
    uint32_t s_feature_ro_compat;   // readonly-compatible feature set
    uint8_t  s_uuid[16];            // 1280bit uuid for volume
    char     s_volume_name[16];     // volume name
    char     s_last_mounted[64];    // directory where last mounted
    uint32_t s_algorithm_usage_bitmap;  // for compression

    /**
     * Performance hints. Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLC flag is on.
     */
    uint8_t  s_prealloc_block;      // Nr of blocks to try to preallocate
    uint8_t  s_prealloc_dir_blocks; // Nr to preallocate or dirs
    uint16_t s_padding1;

    /**
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    uint8_t  s_journal_uuid[16];    // uuid of journal superblock
    uint32_t s_journal_inum;        // inode number of journal file
    uint32_t s_journal_dev;         // device number of journal file
    uint32_t s_last_orphan;         // start of list of inodes to delete
    uint32_t s_hash_seed[4];        // HTREE hash seed
    uint8_t  s_def_hash_version;    // default hash version to use
    uint8_t  s_reserved_char_pad;
    uint16_t s_reserved_word_pad;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;       // first metablock block group
    uint32_t s_reserved[190];       // padding to the end of the block
};

#define EXT2_NDIR_BLOCKS    12
#define EXT2_IND_BLOCK      EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK     (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK     (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS       (EXT2_TIND_BLOCK + 1)

/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
    uint16_t i_mode;            // file mode
    uint16_t i_uid;             // low 17 bits of owner uid
    uint32_t i_size;            // size in bytes
    uint32_t i_atime;           // access time
    uint32_t i_ctime;           // creation time
    uint32_t i_mtime;           // modification time
    uint32_t i_dtime;           // deletion time
    uint16_t i_gid;             // low 16 bits of group id
    uint16_t i_links_count;     // links count
    uint32_t i_blocks;          // blocks count
    uint32_t i_flags;           // file flags
    union {
        struct {
            uint32_t l_i_reserved1;
        } linux1;
        struct {
            uint32_t h_i_translator;
        } hurd1;
        struct {
            uint32_t m_i_reserved1;
        } masix1;
    } osd1;                     // os dependent 1
    uint32_t i_block[EXT2_N_BLOCKS];    // pointers to blocks
    uint32_t i_generation;      // file version (for NFS)
    uint32_t i_file_acl;        // file ACL
    uint32_t i_dir_acl;         // directory ACL
    uint32_t i_faddr;           // fragment address
    union {
        struct {
            uint8_t  l_i_frag;  // fragment number
            uint8_t  l_i_fsize; // fragment size
            uint16_t i_pad1;
            uint16_t l_i_uid_high;  // these 2 fields
            uint16_t l_i_gid_high;  // were reserved2[0]
            uint32_t l_i_reserved2;
        } linux2;
        struct {
            uint8_t  h_i_frag;  /* Fragment number */
            uint8_t  h_i_fsize; /* Fragment size */
            uint16_t h_i_mode_high;
            uint16_t h_i_uid_high;
            uint16_t h_i_gid_high;
            uint32_t h_i_author;
        } hurd2;
        struct {
            uint8_t  m_i_frag;  /* Fragment number */
            uint8_t  m_i_fsize; /* Fragment size */
            uint16_t m_pad1;
            uint32_t m_i_reserved2[2];
        } masix2;
    } osd2;                     // OS dependent 2
};

struct ext2_inode_info {
    struct ext2_inode i_ei;
    uint32_t flags;
};

#define EXT2_ROOTINO  2  /* Root inode */

/*
 * Structure of a directory entry
 */
struct ext2_dir_entry {
    uint32_t inode;     // inode number
    uint16_t rec_len;   // directory entry length;
    uint16_t name_len;  // name length;
    char     name[];    // file name, up to EXT2_NAME_LEN
};

/**
 * THe new version of the directory entry. Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext2_dir_entry_2 {
    uint32_t inode;     // inode number
    uint16_t rec_len;   // directory entry length
    uint8_t  name_len;  // name length
    uint8_t  file_type;
    char     name[];    // file name, up to EXT2_NAME_LEN
};

/**
 * Structure of  blocks group descriptor
 */
struct ext2_group_desc {
    uint32_t bg_block_bitmap;       // blocks bitmap block
    uint32_t bg_inode_bitmap;       // inodes bitmap block
    uint32_t bg_inode_table;        // inodes table block
    uint16_t bg_free_blocks_count;  // free blocks count
    uint16_t bg_free_inodes_count;  // free inodes count
    uint16_t bg_used_dirs_count;    // direcotries count
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
};

/**
 * Macro-instructins used to manage group decriptors
 */
#define EXT2_BLOCKS_PER_GROUP(s)    (EXT2_SB(s)->s_blocks_per_group)
#define EXT2_DESC_PER_BLOCK(s)      (EXT2_SB(s)->s_desc_per_block)
#define EXT2_INODES_PER_GROUP(s)    (EXT2_SB(s)->s_inodes_per_group)
#define EXT2_DESC_PER_BLOCK_BITS(s)  (EXT2_SB(s)->s_desc_per_block_bits)

/**
 * Codes for operating systems
 */
#define EXT2_OS_LINUX       0
#define EXT2_OS_HURD        1
#define EXT2_OS_MASIX       2
#define EXT2_OS_FREEBSD     3
#define EXT2_OS_LITES       4

/**
 * Revison levels
 */
#define EXT2_GOOD_OLD_REV   0       // The good old (original) format
#define EXT2_DYNAMIC_REV    1       // V2 format w/ dynamic inode sizes

#define EXT2_CURRENT_REV    EXT2_CODE_OLD_REV
#define EXT2_MAX_SUPP_REV   EXT2_DYNAMIC_REV

#define EXT2_GOOD_OLD_INODE_SIZE    128

// FIrst non-reserved inode for old ext2 filesystems
#define EXT2_GOOD_OLD_FIRST_INO     11

#define EXT2_HAS_INCOMPAT_FEATURE(sb,mask)      \
    ( EXT2_SB(sb)->s_es->s_feature_incompat & mask )
#define EXT2_HAS_RO_COMPAT_FEATURE(sb,mask)     \
    ( EXT2_SB(sb)->s_es->s_feature_ro_compat & mask )

#define EXT2_FEATURE_INCOMPAT_META_BG       0x0010
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001

#define FT2DT(v)  (v) == 1 ? 8 : \
    (v) == 2 ? 4 : \
    (v) == 3 ? 2 : \
    (v) == 4 ? 6 : \
    (v) == 5 ? 1 : \
    (v) == 6 ? 12 : \
    (v) == 7 ? 10 : 0

static inline ext2_fsblk_t
ext2_group_first_block_no(struct superblock *sb, unsigned long group_no)
{
    return group_no * (ext2_fsblk_t)EXT2_BLOCKS_PER_GROUP(sb) +
            EXT2_SB(sb)->s_es->s_first_data_block;
}

// Filesystem specific operations

int           ext2fs_init(void);
int           ext2_mount(struct inode *, struct inode *);
int           ext2_unmount(struct inode *);
struct inode *ext2_getroot(int, int);
void          ext2_readsb(uint32_t dev, struct superblock *sb);
struct inode *ext2_ialloc(uint32_t dev, uint16_t type);
uint32_t      ext2_balloc(uint32_t dev);
void          ext2_bzero(uint32_t dev, int bno);
void          ext2_bfree(uint32_t dev, uint32_t b);
int           ext2_namecmp(const char *s, const char *t);
int           ext2_direntlookup(struct inode *dp, int inum, struct dirent *dep, size_t *ofp);
uint16_t      ext2_getrootino(void);
long          ext2_getdents(struct file *f, char *data, uint64_t size);

// Inode operations of ext2 filesystem

struct inode *ext2_dirlookup(struct inode *dp, char *name, size_t *off);
void          ext2_iupdate(struct inode *ip);
void          ext2_itrunc(struct inode *ip);
void          ext2_cleanup(struct inode *ip);
uint32_t      ext2_bmap(struct inode *ip, uint32_t bn);
void          ext2_ilock(struct inode *ip);
void          ext2_iunlock(struct inode *ip);
void          ext2_stati(struct inode *ip, struct stat *st);
ssize_t       ext2_readi(struct inode *ip, char *dst, size_t off, size_t n);
ssize_t       ext2_writei(struct inode *ip, char *src, size_t off, size_t n);
int           ext2_dirlink(struct inode *dp, char *name, uint32_t inum, uint16_t type);
int           ext2_unlink(struct inode *dp, uint32_t off);
int           ext2_isdirempty(struct inode *dp);

int           init_ext2fs(void);
int           ext2_fill_inode(struct inode *ip);
#endif
