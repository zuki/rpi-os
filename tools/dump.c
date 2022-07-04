#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>

#define NDIRECT         11

struct superblock {
  uint32_t size;         // Size of file system image (blocks)
  uint32_t nblocks;      // Number of data blocks
  uint32_t ninodes;      // Number of inodes.
  uint32_t nlog;         // Number of log blocks
  uint32_t logstart;     // Block number of first log block
  uint32_t inodestart;   // Block number of first inode block
  uint32_t bmapstart;    // Block number of first free map block
};

struct dinode {
  uint16_t type;                // File type
  uint16_t major;               // Major device number (T_DEV only)
  uint16_t minor;               // Minor device number (T_DEV only)
  uint16_t nlink;               // Number of links to inode in file system
  uint32_t size;                // Size of file (bytes)
  uint32_t mode;                  // file mode
  uint32_t uid;                 // owner's user id
  uint32_t gid;                 // owner's gropu id
  struct timespec atime;        // last accessed time
  struct timespec mtime;        // last modified time
  struct timespec ctime;        // created time
  uint32_t addrs[NDIRECT+2];    // Data block addresses
  char    _dummy[4];
};

// FROM mksd.mk
// The total sd card image is 128 MB, 64 MB for boot sector and 64 MB for file system.
// 以下の単位はセクタ
#define SECTOR_SIZE     512
#define SECTORS         256 * 1024
#define BOOT_OFFSET     2048
#define BOOT_SECTORS    128 * 1024
#define FS_OFFSET       (BOOT_OFFSET) + (BOOT_SECTORS)
#define FS_ADDR         (FS_OFFSET) * (SECTOR_SIZE)

#define BUFSIZE         4096

#define MAXOPBLOCKS     42
#define NINODE          1024
#define FSSIZE          800000
#define BSIZE           512
#define LOGSIZE         (MAXOPBLOCKS*3)
#define IPB             (BSIZE / sizeof(struct dinode))

/* Disk layout:
    [ boot block | sb block | log | dinode blocks | free bit map | data blocks ]
    以下の変数はセクタ単位(512B)
*/
const int nbitmap = FSSIZE / (BSIZE * 8) + 1;
const int ninode  = NINODE / IPB + 1;
const int nlog    = LOGSIZE;

static char *TYPE_NAME[4] = { "-", "DIR", "FILE", "DEV" };
struct superblock sb;

void get_superblock(FILE *f) {
    int n;

    fseek(f, FS_ADDR + SECTOR_SIZE * 1, SEEK_SET);
    n = fread(&sb, sizeof(struct superblock), 1, f);
    if (n != 1) {
        perror("super block read");
        exit(1);
    }
}

void help(const char *prog) {
    fprintf(stderr, "%s type[oslbid] [, arg1 [, arg2]]\n", prog);
    fprintf(stderr, " o: print disk layout\n");
    fprintf(stderr, " s: print superblock\n");
    fprintf(stderr, " l: print log block\n");
    fprintf(stderr, " b: print bitmap\n");
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
        seek = FS_ADDR + SECTOR_SIZE;
        printf("SUPER BLOCK:\n");
        printf(" size     : %d (0x%08x)\n", sb.size, sb.size);
        printf(" nblocks  : %d (0x%08x)\n", sb.nblocks, sb.nblocks);
        printf(" ninode   : %d (0x%08x)\n", sb.ninodes, sb.ninodes);
        printf(" nlog     : %d (0x%08x)\n", sb.nlog, sb.nlog);
        printf(" logstart : %d (0x%08x)\n", sb.logstart, sb.logstart);
        printf(" bmapstart: %d (0x%08x)\n", sb.bmapstart, sb.bmapstart);
        break;
    case 'l':
        seek = FS_ADDR + SECTOR_SIZE * 2;
        fseek(f, seek, SEEK_SET);
        n = fread(buf, sizeof(char), SECTOR_SIZE, f);
        if (n != SECTOR_SIZE) {
            perror("log read");
            exit(1);
        }
        printf("LOG:\n");
        dump_hex(seek, buf, 128);
        break;
    case 'i':
        seek = FS_ADDR + SECTOR_SIZE * sb.inodestart;
        for (i = arg1; i <= arg2; i++) {
            iseek = seek + sizeof(struct dinode) * i;
            fseek(f, iseek, SEEK_SET);
            n = fread(buf, sizeof(struct dinode), 1, f);
            if (n != 1) {
                perror("inode read");
                 exit(1);
            }
            struct dinode *inode = (struct dinode *)buf;
            printf("INODE[%d]: 0x%08x\n", i, iseek);
            printf(" type : 0x%04x (%s)\n", inode->type, TYPE_NAME[inode->type]);
            printf(" major: 0x%04x\n", inode->major);
            printf(" minor: 0x%04x\n", inode->minor);
            printf(" nlink: 0x%04x\n", inode->nlink);
            printf(" size : %d (0x%08x)\n", inode->size, inode->size);
            printf(" mode : 0x%04x\n", inode->mode);
            printf(" atime: 0x%08lx\n", inode->atime.tv_sec);
            printf(" mtime: 0x%08lx\n", inode->mtime.tv_sec);
            printf(" ctime: 0x%08lx\n", inode->ctime.tv_sec);
            for (j = 0; j < NDIRECT+2; j++) {
                if (inode->addrs[j] == 0) break;
                printf(" addrs[%02d]: %d\n", j, inode->addrs[j]);
            }
            printf("\n");
        }
        break;
    case 'b':
        seek = FS_ADDR + SECTOR_SIZE * sb.bmapstart;
        fseek(f, seek, SEEK_SET);
        n = fread(buf, sizeof(char), SECTOR_SIZE, f);
        if (n != SECTOR_SIZE) {
            perror("bitmap read");
            exit(1);
        }
        printf("BITMAP:\n");
        dump_hex(seek, buf, 128);
        break;
    case 'd':
        for (i = 0; i < arg2; i++) {
            seek = FS_ADDR + SECTOR_SIZE * (arg1 + i);
            fseek(f, seek, SEEK_SET);
            n = fread(buf, sizeof(char), SECTOR_SIZE, f);
            if (n != SECTOR_SIZE) {
                perror("data read");
                exit(1);
            }
            printf("SECTOR: %d\n", arg1 + i);
            dump_hex(seek, buf, SECTOR_SIZE);
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

    get_superblock(sdimg);

    if (type == 'o') {
        printf("Boot      : 0x%08x (%d)\n", FS_ADDR, 1);
        printf("SuperBlock: 0x%08x (%d)\n", FS_ADDR + SECTOR_SIZE, 1);
        printf("Log       : 0x%08x (%d)\n", FS_ADDR + SECTOR_SIZE * 2, sb.nlog);
        printf("INode     : 0x%08x (%d)\n", FS_ADDR + SECTOR_SIZE * sb.inodestart, sb.ninodes);
        printf("Bitmap    : 0x%08x (%d)\n", FS_ADDR + SECTOR_SIZE * sb.bmapstart, nbitmap);
        printf("Data      : 0x%08x\n\n", FS_ADDR + SECTOR_SIZE * (sb.bmapstart + nbitmap));
    } else {
         dump(sdimg, type, arg1, arg2);
    }

    fclose(sdimg);

    return 0;
}
