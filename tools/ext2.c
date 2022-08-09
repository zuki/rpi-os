#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>

#define EXT2_NDIR_BLOCKS    12
#define EXT2_IND_BLOCK      EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK     (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK     (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS       (EXT2_TIND_BLOCK + 1)


struct superblock {
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


/*
 * Structure of an inode on the disk
 */
struct dinode {
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

/**
 * Structure of  blocks group descriptor
 */
struct group_desc {
    uint32_t bg_block_bitmap;       // blocks bitmap block
    uint32_t bg_inode_bitmap;       // inodes bitmap block
    uint32_t bg_inode_table;        // inodes table block
    uint16_t bg_free_blocks_count;  // free blocks count
    uint16_t bg_free_inodes_count;  // free inodes count
    uint16_t bg_used_dirs_count;    // direcotries count
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
};


// FROM mksd.mk
// The total sd card image is 128 MB, 64 MB for boot sector and 64 MB for file system.
// 以下の単位はセクタ
#define SECTOR_SIZE     512
#define SECTORS         256 * 1024
#define BOOT_OFFSET     2048
#define BOOT_SECTORS    128 * 1024
#define V6_SECTORS      960 * 1024
#define FS_OFFSET       (BOOT_OFFSET) + (BOOT_SECTORS) + (V6_SECTORS)
#define FS_ADDR         (FS_OFFSET) * (SECTOR_SIZE)

#define SB_OFFSET       1024        // byte単位

#define BUFSIZE         4096

#define NINODE          1024
#define FSSIZE          100000
#define BSIZE           4096

#define GROUP_DESC      2
#define BLK_BITMAP      3
#define INODE_BITMAP    4
#define INODE_TABLE     5
#define INODE_BLOCKS    120
#define DATA_BLOCK      125

struct superblock sb;
unsigned short inode_size;

void get_superblock(FILE *f) {
    int n;

    fseek(f, FS_ADDR + SB_OFFSET, SEEK_SET);
    n = fread(&sb, sizeof(struct superblock), 1, f);
    if (n != 1) {
        perror("super block read");
        exit(1);
    }
    inode_size = sb.s_inode_size;
}

void help(const char *prog) {
    fprintf(stderr, "%s type[oslbid] [, arg1 [, arg2]]\n", prog);
    fprintf(stderr, " o: print disk layout\n");
    fprintf(stderr, " s: print superblock\n");
    fprintf(stderr, " g: print group descriptor\n");
    fprintf(stderr, " b: print block bitmap\n");
    fprintf(stderr, " c: print inode bitmap\n");
    fprintf(stderr, " i: print inode, arg1: 開始inum[. arg2: 終了inum[=開始inum]]\n");
    fprintf(stderr, " d: print data, arg1: 開始セクタ番号, [arg2: セクタ数[=1]]\n");
}

void dump_hex(int start, char *buf, int len) {
    int i, j, idx, pos = start;
    for (i = 0; i < len / 16; i++) {
        printf("%08x: ", pos);
        for (j = 0; j < 16; j += 2) {
            idx = i * 16 + j;
            printf("%02x%02x ", (unsigned char)buf[idx], (unsigned char)buf[idx+1]);
        }
        for (j = 0; j < 16; j++) {
            unsigned char c = buf[i*16+j];
            if (c >= 0x20 && c <= 0x7e)
                printf("%c", c);
            else
                printf(".");
        }
        printf("\n");
        pos += 16;
    }
}

void dump(FILE *f, char type, int arg1, int arg2) {

    int seek, iseek, i, j;
    size_t n;
    char buf[BUFSIZE];

    switch(type) {
    case 's':
        seek = FS_ADDR + BSIZE;
        printf("SUPER BLOCK:\n");
        printf(" inodes_count      : %d (0x%08x)\n", sb.s_inodes_count, sb.s_inodes_count);
        printf(" blocks_count      : %d (0x%08x)\n", sb.s_blocks_count, sb.s_blocks_count);
        printf(" r_block_count     : %d (0x%08x)\n", sb.s_r_blocks_count, sb.s_r_blocks_count);
        printf(" free_blocks_count : %d (0x%08x)\n", sb.s_free_blocks_count, sb.s_free_blocks_count);
        printf(" free_inodes_count : %d (0x%08x)\n", sb.s_free_inodes_count, sb.s_free_inodes_count);
        printf(" first_data_block  : %d (0x%08x)\n", sb.s_first_data_block, sb.s_first_data_block);
        printf(" log_block_size    : %d (0x%08x)\n", sb.s_log_block_size, sb.s_log_block_size);
        printf(" log_frag_size     : %d (0x%08x)\n", sb.s_log_frag_size, sb.s_log_frag_size);
        printf(" blocks_per_group  : %d (0x%08x)\n", sb.s_blocks_per_group, sb.s_blocks_per_group);
        printf(" frags_per_group   : %d (0x%08x)\n", sb.s_frags_per_group, sb.s_frags_per_group);
        printf(" indoes_per_group  : %d (0x%08x)\n", sb.s_indoes_per_group, sb.s_indoes_per_group);
        printf(" max_mnt_count     : %d (0x%04x)\n", sb.s_max_mnt_count, sb.s_max_mnt_count);
        printf(" s_magic           : %d (0x%04x)\n", sb.s_magic, sb.s_magic);
        printf(" s_state           : %d (0x%04x)\n", sb.s_state, sb.s_state);
        printf(" minor_rev_level   : %d (0x%04x)\n", sb.s_minor_rev_level, sb.s_minor_rev_level);
        printf(" rev_level         : %d (0x%08x)\n", sb.s_rev_level, sb.s_rev_level);
        printf("\n");
        printf(" first_ino         : %d (0x%08x)\n", sb.s_first_ino, sb.s_first_ino);
        printf(" inode_size        : %d (0x%04x)\n", sb.s_inode_size, sb.s_inode_size);
        printf(" block_group_nr    : %d (0x%04x)\n", sb.s_block_group_nr, sb.s_block_group_nr);
        printf(" feature_compat    : %d (0x%08x)\n", sb.s_feature_compat, sb.s_feature_compat);
        printf(" feature_incompat  : %d (0x%08x)\n", sb.s_feature_incompat, sb.s_feature_incompat);
        printf(" feature_ro_compat : %d (0x%08x)\n", sb.s_feature_ro_compat, sb.s_feature_ro_compat);
        printf(" uuid              :");
        for (int i = 0; i < 16; i++) {
            printf("%02x", sb.s_uuid[i]);
        }
        printf("\n");
        printf(" volume_name       :");
        for (int i = 0; i < 16; i++) {
            printf("%02x", sb.s_volume_name[i]);
        }
        printf("\n");
        break;
    case 'i':
        seek = FS_ADDR + BSIZE * INODE_TABLE;
        for (i = arg1 - 1; i < arg2; i++) {
            iseek = seek + inode_size * i;
            fseek(f, iseek, SEEK_SET);
            n = fread(buf, inode_size, 1, f);
            if (n != 1) {
                perror("inode read");
                 exit(1);
            }
            struct dinode *inode = (struct dinode *)buf;
            printf("INODE[%d]: 0x%08x\n", i, iseek);
            printf(" mode  : 0x%04x\n", inode->i_mode);
            printf(" uid   : 0x%04x\n", inode->i_uid);
            printf(" gid   : 0x%04x\n", inode->i_gid);
            printf(" size  : 0x%04x\n", inode->i_size);
            printf(" atime : 0x%08x\n", inode->i_atime);
            printf(" mtime : 0x%08x\n", inode->i_mtime);
            printf(" dtime : 0x%08x\n", inode->i_dtime);
            printf(" links : 0x%04x\n", inode->i_links_count);
            printf(" blocks: 0x%08x\n", inode->i_blocks);
            printf(" flags : 0x%08x\n", inode->i_flags);
            if (*inode->i_block) {
                for (j = 0; j < EXT2_N_BLOCKS; j++) {
                    if (inode->i_block[j] == 0) break;
                    printf(" iblock[%02d]: %d\n", j, inode->i_block[j]);
                }
                printf("\n");
            }
        }
        break;
    case 'g':
        seek = FS_ADDR + BSIZE * GROUP_DESC;
        fseek(f, seek, SEEK_SET);
        n = fread(buf, sizeof(struct group_desc), 1, f);
        if (n != 1) {
            perror("group desc read");
            exit(1);
        }
        struct group_desc *gdesc = (struct group_desc *)buf;
        printf("GROUP DESC: 0x%08x\n", seek);
        printf(" block_bitmap      : 0x%08x\n", gdesc->bg_block_bitmap);
        printf(" inode_table       : 0x%08x\n", gdesc->bg_inode_table);
        printf(" free_blocks_count : 0x%08x\n", gdesc->bg_free_blocks_count);
        printf(" free_inodes_count : 0x%04x\n", gdesc->bg_free_inodes_count);
        printf(" used_dirs_count   : 0x%04x\n", gdesc->bg_used_dirs_count);
        printf(" block_bitmap      : 0x%04x\n", gdesc->bg_block_bitmap);
        break;
    case 'b':
        seek = FS_ADDR + BSIZE * BLK_BITMAP;
        fseek(f, seek, SEEK_SET);
        n = fread(buf, sizeof(char), BSIZE, f);
        if (n != BSIZE) {
            perror("bitmap read");
            exit(1);
        }
        printf("BLOCK BITMAP:\n");
        dump_hex(seek, buf, 128);
        break;
    case 'c':
        seek = FS_ADDR + BSIZE * INODE_BITMAP;
        fseek(f, seek, SEEK_SET);
        n = fread(buf, sizeof(char), BSIZE, f);
        if (n != BSIZE) {
            perror("bitmap read");
            exit(1);
        }
        printf("INODE BITMAP:\n");
        dump_hex(seek, buf, 128);
        break;
    case 'd':
        for (i = 0; i < arg2; i++) {
            seek = FS_ADDR + BSIZE * (arg1 + i);
            fseek(f, seek, SEEK_SET);
            n = fread(buf, sizeof(char), BSIZE, f);
            if (n != BSIZE) {
                perror("data read");
                exit(1);
            }
            printf("SECTOR: %d\n", arg1 + i);
            dump_hex(seek, buf, 512);
        }
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[]) {
    char type;
    int arg1, arg2;
    FILE *sdimg;

    // type [s, l, i, b], number
    if (argc < 2) {
        help(argv[0]);
        return 1;
    }

    type = *argv[1];
    if (type == 'i') {
        if (argc < 3) {
            help(argv[0]);
            return 1;
        } else {
            arg1 = arg2 = atoi(argv[2]);
            if (argc == 4)
                arg2 = atoi(argv[3]);
        }
    }

    if (type == 'd') {
        if (argc < 3) {
            help(argv[0]);
            return 1;
        } else {
            arg1 = atoi(argv[2]);
            if (argc == 4)
                arg2 = atoi(argv[3]);
            else
                arg2 = 1;
        }
    }

    if ((sdimg = fopen("../obj/sd.img", "r")) == NULL) {
        perror("open sd.img");
        return 1;
    }

    //printf("sizeof inode=0x%lx\n", sizeof(struct dinode));
    get_superblock(sdimg);

    if (type == 'o') {
        printf("SuperBlock      : 0x%08x (%d)\n", FS_ADDR + SB_OFFSET, 1);
        printf("Group Descripter: 0x%08x (%d)\n", FS_ADDR + BSIZE * 1, 1);
        printf("Block Bitmap    : 0x%08x (%d)\n", FS_ADDR + BSIZE * 3, 1);
        printf("Inode Bitmap    : 0x%08x (%d)\n", FS_ADDR + BSIZE * 4, 1);
        printf("INode Table     : 0x%08x (%d)\n", FS_ADDR + BSIZE * 5, 120);
        printf("Data            : 0x%08x\n\n", FS_ADDR + BSIZE * 125);
    } else {
         dump(sdimg, type, arg1, arg2);
    }

    fclose(sdimg);

    return 0;
}
