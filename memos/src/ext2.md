# Ext2ファイルシステムをサポート

- [caiolima/xv6-public](https://github.com/caiolima/xv6-public)を使用
- mkfs.ext2はhomebrewのe2fsprogsをインストールして使用

## 問題1: 書き込みができない

```
デバイス    起動 開始位置 終了位置 セクタ サイズ Id タイプ
obj/sd.img1          2048   133119 131072    64M  c W95 FAT32 (LBA)
obj/sd.img2        133120  1116159 983040   480M 83 Linux
obj/sd.img3       1116160  2097151 980992   479M 83 Linux

パーティション情報が変更されました。
ディスクを同期しています。
dd if=obj/boot.img of=obj/sd.img seek=2048 conv=notrunc
131072+0 records in
131072+0 records out
67108864 bytes transferred in 0.800265 secs (83858319 bytes/sec)
dd if=obj/fs.img of=obj/sd.img seek=133120 conv=notrunc
800000+0 records in
800000+0 records out
409600000 bytes transferred in 4.852061 secs (84417734 bytes/sec)
dd if=obj/ext2.img of=obj/sd.img seek=1116160 conv=notrunc
61440+0 records in
61440+0 records out
31457280 bytes transferred in 0.377347 secs (83364332 bytes/sec)
qemu-system-aarch64 -M raspi3b -nographic -serial null -serial mon:stdio -drive file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
[3]free_range: 0xffff00000033b000 ~ 0xffff00003b400000, 241861 pages
[3]console_init: devsw[1].termios: 0xffff00003b3ff000
[3]rand_init: rand_init ok
[3]init_vfssw: init_vfssw ok
mountinit ok
[3]install_rootfs: install_rootfs ok
pagecache_init ok
[1]main: cpu 1 init finished
[2]main: cpu 2 init finished
[3]main: cpu 3 init finished
[0]main: cpu 0 init finished
[2]emmc_card_init: poweron
[2]emmc_issue_command_int: rrror occured whilst waiting for command complete interrupt
[2]emmc_card_reset: found valid version 2.00 SD card
[2]dev_init: partition[0]: LBA=0x800, #SECS=0x20000
[2]dev_init: partition[1]: LBA=0x20800, #SECS=0xf0000
[2]dev_init: partition[2]: LBA=0x110800, #SECS=0xef800
[2]iinit: sb: size 100000 nblocks 99835 ninodes 1024 nlog 126 logstart 2 inodestart 128 bmapstart 161
[2]initlog: init log ok
[3]mount: source: /dev/sdc3, target: /mnt, type: ext2, flags: 0x0
[3]bget: acqsleep2: bno=0     // bno=0: スーバーブロック
[3]brelse: relsleep: bno=0
[3]bget: acqsleep1: bno=0
[3]bget: acqsleep2: bno=1     // bno=1: グループ記述子: ここでロック確保
[3]ext2_readsb: sb: major=0, minor=2, blksize=4096, lba=0x110800, nescs: 0xef800, bits=12, flags=1
[3]ext2_readsb: sbi: s_inodes_per_block=16
[3]ext2_readsb: sbi: s_blocks_per_group=32768
[3]ext2_readsb: sbi: s_inodes_per_group=1920
[3]ext2_readsb: sbi: s_itb_per_group=120
[3]ext2_readsb: sbi: s_gdb_count=1
[3]ext2_readsb: sbi: s_desc_per_block=128
[3]ext2_readsb: sbi: s_groups_count=1
[3]ext2_readsb: sbi: s_overhead_last=0
[3]ext2_readsb: sbi: s_blocks_last=0
[3]ext2_readsb: sbi: s_sbh=0xffff00000012e120
[3]ext2_readsb: sbi: s_es=0xffff00000012e530
[3]ext2_readsb: sbi: s_group_desc=0xffff00000012d0c8
[3]ext2_readsb: sbi: s_sb_block=0
[3]ext2_readsb: sbi: s_addr_per_block_bits=10
[3]ext2_readsb: sbi: s_desc_per_block_bits=7
[3]ext2_readsb: sbi: s_inode_size=256
[3]ext2_readsb: sbi: s_first_ino=11
[3]ext2_readsb: sbi: s_dir_count=0
[3]bget: acqsleep2: bno=5           // bno=5: inodeテーブル
[3]brelse: relsleep: bno=5

[2]fileopen: cant namei /etc/issue
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: root
Password:
[2]fileopen: cant namei /etc/profile
[2]fileopen: cant namei /.profile
# cd /mnt
[2]bget: acqsleep2: bno=5
[2]brelse: relsleep: bno=5
# cat > test
[3]bget: acqsleep2: bno=125       // ino=2のデータ領域(/mntディレクトリ)
[3]brelse: relsleep: bno=125
[3]bget: acqsleep1: bno=125
[3]brelse: relsleep: bno=125
[3]bget: acqsleep1: bno=125
[3]brelse: relsleep: bno=125
[3]create: path=test, uid=0, gid=0
[3]create: get dp
[3]create: get ilock
[3]create: get permission
[3]bget: acqsleep1: bno=125
[3]brelse: relsleep: bno=125
[3]bget: acqsleep1: bno=125
[3]brelse: relsleep: bno=125
[3]bget: acqsleep1: bno=125
[3]brelse: relsleep: bno=125
[3]create: dirlookup
[3]bget: acqsleep2: bno=4           // bno=4: inodeビットマップ
[3]ext2_ialloc: bwrite bitmap_bh: blcokno=0x4
[3]brelse: relsleep: bno=4
[3]ext2_ialloc: bwrite bh2: blcokno=0x1     // <= グループ記述子（
bwrite: not holdingdsleep dev: 2, blockno: 0x1
kern/console.c:348: kernel panic at cpu 3.
QEMU: Terminated
```

```
# break ext2.c:523
(gdb) p/x *bh2
$1 = {
  flags = 0x2,
  dev = 0x2,
  blockno = 0x1,
  refcnt = 0x1,
  data = {0x3, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0x7d,
    0x1d, 0x74, 0x7, 0x2, 0x0 <repeats 4079 times>},
  lock = {
    locked = 0x1,
    lk = {
      locked = 0x0,
      name = 0xffff0000000aa3f0
    },
    pid = 0xb,                        // pid が違う: [11] = /bin/mount
    name = 0xffff0000000abac8
  },
  clink = {
    next = 0xffff0000001332b8,
    prev = 0xffff00000012c050
  },
  dlink = {
    next = 0xffff000000286eb0,
    prev = 0xffff000000286eb0
  }
}
(gdb) p/x thisproc()->pid
$3 = 0xc                              // [12] = /bin/dash
(gdb) p thisproc()->name
$5 = "dash", '\000' <repeats 11 times>
```

### プロセスをまたぐsleeplockは持てないのでsleeplockを諦める

- sleeplockの代わりにB_BUSYフラグを使用

## ext2_dirlink()でストール

- breakしたときには設定されていたclink, dlinkが途中でヌルに

```
1837	        bh = ext2_ops.bread(dp->dev, ext2_iops.bmap(dp, n));
(gdb) p/x *bh
$1 = {
  flags = 0x3,
  dev = 0x2,
  blockno = 0x7d,
  data = {0x2, 0x0, 0x0, 0x0, 0xc, 0x0, 0x1, 0x2, 0x2e, 0x0, 0x0, 0x0, 0x2,
    0x0, 0x0, 0x0, 0xc, 0x0, 0x2, 0x2, 0x2e, 0x2e, 0x0, 0x0, 0xb, 0x0, 0x0,
    0x0, 0xe8, 0xf, 0xa, 0x2, 0x6c, 0x6f, 0x73, 0x74, 0x2b, 0x66, 0x6f, 0x75,
    0x6e, 0x64, 0x0 <repeats 4054 times>},
  clink = {
    next = 0xffff000000136088,
    prev = 0xffff000000135038
  },
  dlink = {
    next = 0xffff000000285b00,
    prev = 0xffff000000285b00
  }
}
(gdb)
1901	    ext2_ops.bwrite(bh);
(gdb) s
bwrite (b=0xffff00000012ae78 <bcache+476576>) at kern/bio.c:125
125	    if ((b->flags & B_BUSY) == 0) {
(gdb) p/x *b
$7 = {
  flags = 0x3,
  dev = 0x2,
  blockno = 0x7d,
  data = {0x2, 0x0, 0x0, 0x0, 0xc, 0x0, 0x1, 0x2, 0x2e, 0x0, 0x0, 0x0, 0x2,
    0x0, 0x0, 0x0, 0xc, 0x0, 0x2, 0x2, 0x2e, 0x2e, 0x0, 0x0, 0xb, 0x0, 0x0,
    0x0, 0x14, 0x0, 0xa, 0x2, 0x6c, 0x6f, 0x73, 0x74, 0x2b, 0x66, 0x6f, 0x75,
    0x6e, 0x64, 0x0, 0x0, 0x10, 0x0, 0x0, 0x0, 0xd4, 0xf, 0x4, 0x1, 0x74,
    0x65, 0x73, 0x74, 0x0 <repeats 4040 times>},
  clink = {                                   // ヌルになっている
    next = 0x0,
    prev = 0x0
  },
  dlink = {                                   // ヌルになっている
    next = 0x0,
    prev = 0x0
  }
}
(gdb) n
129	    b->flags |= B_DIRTY;
(gdb)
130	    devrw(b);
(gdb) s
devrw (b=0xffff00000012ae78 <bcache+476576>) at kern/dev.c:132
132	    acquire(&cardlock);
(gdb) p/x b->blockno
$8 = 0x7d
(gdb) n
135	    list_push_back(&devque, &b->dlink);   // &b->dlinkがヌル
(gdb)
```

### 解決: この前の方にあるstrncpy()

- この第３パラメタがunsigned longになっていてメモリ領域を破壊
- signed longに変更

```c
static inline char *
strncpy(char *s, const char *t, ssize_t n)      // このnがsize_tになっていて
{
    char *os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)       // ここを抜けた際の n = -1
        ;
    while (n-- > 0)                             // unsignedなので n = 0xffffffffffffffff と
        *s++ = 0;                               // 判断され、*s以降を破壊していた
    return os;
}
```

### 実行

```
デバイス    起動 開始位置 終了位置 セクタ サイズ Id タイプ
obj/sd.img1          2048   133119 131072    64M  c W95 FAT32 (LBA)
obj/sd.img2        133120  1116159 983040   480M 83 Linux
obj/sd.img3       1116160  2097151 980992   479M 83 Linux

パーティション情報が変更されました。
ディスクを同期しています。
dd if=obj/boot.img of=obj/sd.img seek=2048 conv=notrunc
131072+0 records in
131072+0 records out
67108864 bytes transferred in 0.807705 secs (83085867 bytes/sec)
dd if=obj/fs.img of=obj/sd.img seek=133120 conv=notrunc
800000+0 records in
800000+0 records out
409600000 bytes transferred in 4.513877 secs (90742395 bytes/sec)
dd if=obj/ext2.img of=obj/sd.img seek=1116160 conv=notrunc
61440+0 records in
61440+0 records out
31457280 bytes transferred in 0.306695 secs (102568616 bytes/sec)
qemu-system-aarch64 -M raspi3b -nographic -serial null -serial mon:stdio -drive file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
[0]free_range: 0xffff000000339000 ~ 0xffff00003b400000, 241863 pages
[0]console_init: devsw[1].termios: 0xffff00003b3ff000
[0]rand_init: rand_init ok
[0]init_vfssw: init_vfssw ok
mountinit ok
[0]install_rootfs: install_rootfs ok
pagecache_init ok
[2]main: cpu 2 init finished
[1]main: cpu 1 init finished
[3]main: cpu 3 init finished
[0]main: cpu 0 init finished
[3]emmc_card_init: poweron
[3]emmc_issue_command_int: rrror occured whilst waiting for command complete interrupt
[3]emmc_card_reset: found valid version 2.00 SD card
[3]dev_init: partition[0]: LBA=0x800, #SECS=0x20000
[3]dev_init: partition[1]: LBA=0x20800, #SECS=0xf0000
[3]dev_init: partition[2]: LBA=0x110800, #SECS=0xef800
[3]iinit: sb: size 100000 nblocks 99835 ninodes 1024 nlog 126 logstart 2 inodestart 128 bmapstart 161
[3]initlog: init log ok
[2]mount: source: /dev/sdc3, target: /mnt, type: ext2, flags: 0x0

[2]fileopen: cant namei /etc/issue
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: root
Password:
[1]fileopen: cant namei /etc/profile
[2]fileopen: cant namei /.profile
# ls -il /mnt
total 3
11 drwxrwxr-x 2 root root 16384 Aug  9  2022 lost+found
# cd /mnt
# cat > test
mount test
# cat test
mount test
# ls -il .
total 3
11 drwxrwxr-x 2 root root 16384 Aug  9  2022 lost+found
14 drwxrwxr-x 1 root root    11 Jun 21 10:31 test
# cd /
# ls -il /mnt
total 3
11 drwxrwxr-x 2 root root 16384 Aug  9  2022 lost+found
14 drwxrwxr-x 1 root root    11 Jun 21 10:31 test
# umount /mnt
# ls -il /mnt
total 0
# mount /dev/sdc3 /mnt ext2
[2]mount: source: /dev/sdc3, target: /mnt, type: ext2, flags: 0x0
# ls -il /mnt
total 3
11 drwxrwxr-x 2 root root 16384 Aug  9  2022 lost+found
14 drwxrwxr-x 1 root root    11 Jun 21 10:31 test
# cat /mnt/test
mount test
# cat > test2
root dir test
# ls -il /
total 5
  2 drwxrwxr-x 1 root root 1600 Aug  9  2022 bin
  3 drwxrwxr-x 1 root root  384 Aug  9  2022 dev
  8 drwxrwxr-x 1 root root  320 Aug  9  2022 etc
 11 drwxrwxr-x 1 root root  192 Aug  9  2022 home
  9 drwxrwxrwx 1 root root  256 Aug  9  2022 lib
  2 drwxrwxr-x 3 root root 4096 Aug  9  2022 mnt
 45 -rwxr-xr-x 1 root root   94 Aug  9  2022 test.txt
192 -rw-rw-r-- 1 root root   14 Jun 21 10:33 test2
 14 drwxrwxr-x 1 root root  256 Aug  9  2022 usr
# cat test2
root dir test
# rm test2
# rm /mnt/test
# ls -il /mnt
total 4
 2 drwxrwxr-x 3 root root  4096 Aug  9  2022 ''
11 drwxrwxr-x 2 root root 16384 Aug  9  2022  lost+found
# umount /mnt
# ls -l /mnt
total 0
#
```

## `dump22fs`コマンド

```
$ /usr/local/opt/e2fsprogs/sbin/dumpe2fs ext2.img
dumpe2fs 1.46.4 (18-Aug-2021)
Filesystem volume name:   <none>
Last mounted on:          <not available>
Filesystem UUID:          63e9227b-7e3c-46cf-9c52-c327144efba8
Filesystem magic number:  0xEF53
Filesystem revision #:    1 (dynamic)
Filesystem features:      ext_attr resize_inode dir_index filetype sparse_super large_file
Filesystem flags:         signed_directory_hash
Default mount options:    user_xattr acl
Filesystem state:         clean
Errors behavior:          Continue
Filesystem OS type:       Linux
Inode count:              1920
Block count:              7680
Reserved block count:     384
Overhead clusters:        125
Free blocks:              7549
Free inodes:              1909
First block:              0
Block size:               4096
Fragment size:            4096
Reserved GDT blocks:      1
Blocks per group:         32768
Fragments per group:      32768
Inodes per group:         1920
Inode blocks per group:   120
Filesystem created:       Mon Aug  8 10:45:40 2022
Last mount time:          n/a
Last write time:          Mon Aug  8 10:45:40 2022
Mount count:              0
Maximum mount count:      -1
Last checked:             Mon Aug  8 10:45:40 2022
Check interval:           0 (<none>)
Reserved blocks uid:      0 (user root)
Reserved blocks gid:      0 (group wheel)
First inode:              11
Inode size:	          256
Required extra isize:     32
Desired extra isize:      32
Default directory hash:   half_md4
Directory Hash Seed:      e9861380-073a-46cd-acd8-84de87aa4749


Group 0: (Blocks 0-7679)
  Primary superblock at 0, Group descriptors at 1-1
  Reserved GDT blocks at 2-2
  Block bitmap at 3 (+3)
  Inode bitmap at 4 (+4)
  Inode table at 5-124 (+5)
  7549 free blocks, 1909 free inodes, 2 directories
  Free blocks: 131-7679
  Free inodes: 12-1920
```

## fudan

```
# ls -li /mnt
total 17
?'Ddrwxrwxr-x 2 root root 16384 Aug  6  2022 lost+found
11 drwxrwxr-x 2 root root 16384 Aug  6  2022 lost+found
14 drwxrwxr-x 1 root root    11 Sep 30  2021 test
# umount /mnt
```

```
$ cd tools/
$ ./ext2 i 14
INODE[13]: 0x1d105d00
 mode  : 0x8000
 uid   : 0x0000
 gid   : 0x0000
 size  : 0x000b
 atime : 0x6155028b
 mtime : 0x6155028b
 dtime : 0x00000000
 links : 0x0001
 blocks: 0x00000000
 flags : 0x00000000
 iblock[00]: 131

$ ./ext2 d 131
SECTOR: 131
1d183000: 6d6f 756e 7420 7465 7374 0a00 0000 0000 mount test......
1d183010: 0000 0000 0000 0000 0000 0000 0000 0000 ................

$ ./ext2 i 11
INODE[10]: 0x1d105a00
 mode  : 0x41c0
 uid   : 0x0000
 gid   : 0x0000
 size  : 0x4000
 atime : 0x62eef75e
 mtime : 0x62eef75e
 dtime : 0x00000000
 links : 0x0002
 blocks: 0x00000020
 flags : 0x00000000
 iblock[00]: 126
 iblock[01]: 127
 iblock[02]: 128
 iblock[03]: 129

$ ./ext2 d 126
SECTOR: 126
1d17e000: 0b00 0000 0c00 0102 2e00 0000 0200 0000 ................
1d17e010: f40f 0202 2e2e 0000 0000 0000 0000 0000 ................
1d17e020: 0000 0000 0000 0000 0000 0000 0000 0000 ................

$ ./ext2 s
SUPER BLOCK:
 inodes_count      : 1920 (0x00000780)
 blocks_count      : 7680 (0x00001e00)
 r_block_count     : 384 (0x00000180)
 free_blocks_count : 7549 (0x00001d7d)
 free_inodes_count : 1909 (0x00000775)
 first_data_block  : 0 (0x00000000)
 log_block_size    : 2 (0x00000002)
 log_frag_size     : 2 (0x00000002)
 blocks_per_group  : 32768 (0x00008000)
 frags_per_group   : 32768 (0x00008000)
 indoes_per_group  : 1920 (0x00000780)
 max_mnt_count     : 65535 (0xffff)
 s_magic           : 61267 (0xef53)
 s_state           : 1 (0x0001)
 minor_rev_level   : 0 (0x0000)
 rev_level         : 1 (0x00000001)

 first_ino         : 11 (0x0000000b)
 inode_size        : 256 (0x0100)
 block_group_nr    : 0 (0x0000)
 feature_compat    : 56 (0x00000038)
 feature_incompat  : 2 (0x00000002)
 feature_ro_compat : 3 (0x00000003)
 uuid              :118a785fc5b8416bb89d16dceb390d9d
 volume_name       :00000000000000000000000000000000
$ ./ext2 b
BLOCK BITMAP:
1d103000: ffff ffff ffff ffff ffff ffff ffff ffff ................
1d103010: 0f00 0000 0000 0000 0000 0000 0000 0000 ................
1d103020: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d103030: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d103040: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d103050: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d103060: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d103070: 0000 0000 0000 0000 0000 0000 0000 0000 ................
$ ./ext2 c
INODE BITMAP:
1d104000: ff27 0000 0000 0000 0000 0000 0000 0000 .'..............
1d104010: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d104020: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d104030: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d104040: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d104050: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d104060: 0000 0000 0000 0000 0000 0000 0000 0000 ................
1d104070: 0000 0000 0000 0000 0000 0000 0000 0000 ................
$ ./ext2 g
GROUP DESC: 0x1d102000
 block_bitmap      : 0x00000000
 inode_table       : 0x00000000
 free_blocks_count : 0x00000000
 free_inodes_count : 0x0000
 used_dirs_count   : 0x0000
 block_bitmap      : 0x0000
```

## 更新履歴 (vfs, ext2, mountを含む)

```
$ git diff --compact-summary ca02803
 README.md                   |    3 +
 inc/buf.h                   |    8 +-
 inc/dev.h                   |   12 +
 inc/ext2.h (new)            |  368 ++++++++
 inc/file.h                  |   51 +-
 inc/find_bits.h (new)       |  162 ++++
 inc/fs.h (gone)             |  115 ---
 inc/linux/find_bits.h (new) |  162 ++++
 inc/linux/ilog2.h (new)     |  108 +++
 inc/log.h                   |    6 +-
 inc/string.h                |   24 +-
 inc/types.h                 |    3 +-
 inc/v6.h (new)              |  105 +++
 inc/vfs.h (new)             |  238 +++++
 inc/vfsmount.h (new)        |   31 +
 kern/bio.c                  |   43 +-
 kern/console.c              |   13 +-
 kern/dev.c                  |   38 +-
 kern/exec.c                 |   15 +-
 kern/ext2.c (new)           | 2183 +++++++++++++++++++++++++++++++++++++++++++
 kern/file.c                 |  280 ++++--
 kern/find_bits.c (new)      |   42 +
 kern/fs.c (gone)            |  958 -------------------
 kern/log.c                  |  213 +++--
 kern/main.c                 |   11 +-
 kern/mm.c                   |    2 +-
 kern/mmap.c                 |   16 +-
 kern/pagecache.c            |    7 +-
 kern/pipe.c                 |    2 +-
 kern/syscall.c              |   12 +-
 kern/sysfile.c              |   61 +-
 kern/trap.c                 |    2 +-
 kern/v6.c (new)             |  725 ++++++++++++++
 kern/vfs.c (new)            |  661 +++++++++++++
 kern/vfsmount.c (new)       |   71 ++
 kern/vm.c                   |    4 +-
 memos/src/SUMMARY.md        |    3 +
 memos/src/ext2.md (new)     |  504 ++++++++++
 memos/src/mount.md (new)    |   10 +
 memos/src/vfs.md (new)      |  241 +++++
 mksd.mk                     |   25 +-
 tools/dump.c                |    2 +-
 tools/ext2 (new +x)         |  Bin 0 -> 50264 bytes
 tools/ext2.c (new)          |  424 +++++++++
 usr/etc/inittab             |    1 +
 usr/src/mkfs/main.c         |    6 +-
 usr/src/mount/main.c (new)  |   18 +
 usr/src/umount/main.c (new) |   18 +
 48 files changed, 6606 insertions(+), 1401 deletions(-)
```
