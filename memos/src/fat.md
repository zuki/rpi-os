# FAT32ファイルシステム

## MBR

```
[3]dev_init: partition[0]: LBA=0x800, #SECS=0x20000
[3]dev_init: partition[1]: LBA=0x20800, #SECS=0xf0000
[3]dev_init: partition[2]: LBA=0x110800, #SECS=0xef800
```

## sd.img内のFAT32 fs

参考: [FATファイルシステムのしくみと操作法](http://elm-chan.org/docs/fat.html)

```
FAT : 0x100000 (512 * LBA)

# 予約領域

00100000: eb58 904d 544f 4f34 3033 3800 0201 2000  .X.MTOO4038... .
00100010: 0200 0000 00f8 0000 3f00 1000 0000 0000  ........?.......
00100020: 0000 0200 f103 0000 0000 0000 0200 0000  ................
00100030: 0100 0600 0000 0000 0000 0000 0000 0000  ................
00100040: 0000 290a 2445 024e 4f20 4e41 4d45 2020  ..).$E.NO NAME
00100050: 2020 4641 5433 3220 2020 fa31 c08e d88e    FAT32   .1....
00100060: c0fc b900 01be 007c bf00 80f3 a5ea 7200  .......|......r.
00100070: 0008 b801 02bb 007c ba80 00b9 0100 cd13  .......|........
00100080: 7205 ea00 7c00 00cd 1900 0000 0000 0000  r...|...........

001001c0: 0100 0c0f 3f82 0000 0000 d003 0200 0000  ....?...........
001001d0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
001001e0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
001001f0: 0000 0000 0000 0000 0000 0000 0000 55aa  ..............U.

BS_JmpBoot     : 0xeb5890
BS_OEMName     : 4d544f4f34303338 : MTOO4038
BPB_BytsPerSec : 0x200
BPB_SecPerClus : 1
BPB_RsvdSecCnt : 0x20
BPB_NumFATs    : 2
BPB_RootEntCnt : 0
BPB_TotSec16   : 0
BPB_Media      : f8
BPB_FATSz16    : 0
BPB_SecPerTrk  : 0x3f
BPB_NumHeads   : 0x10
BPB_HiddSec    : 0
BPB_TotSec32   : 0x20000

BPB_FATSz32    : 0x03f1
BPB_ExtFlags   : 0
BPB_FSVer      : 0                      : v0.0
BPB_RootClus   : 2
BPB_FSInfo     : 1
BPB_BkBootSec  : 6
BPB_Reserved   : 0
BS_DrvNum      : 0
BS_Reserved    : 0
BS_BootSig     : 0x29
BS_VolID       : 0x0245240a
BS_VolLab      : 4e4f204e414d4520202020	: NO NAME
BS_FilSysType  : 4641543332202020       : FAT32
BS_BootCode32  :
00100050:                          fa31 c08e d88e
00100060: c0fc b900 01be 007c bf00 80f3 a5ea 7200
00100070: 0008 b801 02bb 007c ba80 00b9 0100 cd13
00100080: 7205 ea00 7c00 00cd 1900

# FSInfo

00100200: 5252 6141 0000 0000 0000 0000 0000 0000  RRaA............
...
001003e0: 0000 0000 7272 4161 eac1 0100 1536 0000  ....rrAa.....6..
001003f0: 0000 0000 0000 0000 0000 0000 0000 55aa  ..............U.

FSI_LeadSig    : 0x41615252
FSI_Reserved1  : 0
FSI_StrucSig   : 0x61417272
FSI_Free_Count : 0x1c1ea
FSI_Nxt_Free   : 0x3615         : 0x104000 + 0x3615 * 4
FSI_Reserved2  : 0
FSI_TrailSig   : 0xaa550000

# ブートセクタのバックアップ (戦闘はBPB_BkBootSec=6セクタ)

00100c00: eb58 904d 544f 4f34 3033 3800 0201 2000  .X.MTOO4038... .
00100c10: 0200 0000 00f8 0000 3f00 1000 0000 0000  ........?.......
00100c20: 0000 0200 f103 0000 0000 0000 0200 0000  ................
00100c30: 0100 0600 0000 0000 0000 0000 0000 0000  ................
00100c40: 0000 290a 2445 024e 4f20 4e41 4d45 2020  ..).$E.NO NAME
00100c50: 2020 4641 5433 3220 2020 fa31 c08e d88e    FAT32   .1....
00100c60: c0fc b900 01be 007c bf00 80f3 a5ea 7200  .......|......r.
00100c70: 0008 b801 02bb 007c ba80 00b9 0100 cd13  .......|........
00100c80: 7205 ea00 7c00 00cd 1900 0000 0000 0000  r...|...........

# FAT1 : 0x104000 - (0x3f1セクタ)

00104000: f8ff ffff ffff ff0f 5502 0000 0400 0000  ........U.......
00104010: 0500 0000 0600 0000 0700 0000 0800 0000  ................
00104020: 0900 0000 0a00 0000 0b00 0000 0c00 0000  ................
00104030: 0d00 0000 0e00 0000 0f00 0000 1000 0000  ................
...
00104640: 9101 0000 9201 0000 ffff ff0f 9401 0000  ................
00104650: 9501 0000 9601 0000 9701 0000 9801 0000  ................
00104660: 9901 0000 9a01 0000 9b01 0000 9c01 0000  ................
...
00111840: 1136 0000 1236 0000 1336 0000 1436 0000  .6...6...6...6..
00111850: 1536 0000 ffff ff0f 0000 0000 0000 0000  .6..............
00111860: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00111870: 0000 0000 0000 0000 0000 0000 0000 0000  ................


# FAT2 : 0x182200 - (0x3f1セクタ)

00182200: f8ff ffff ffff ff0f 5502 0000 0400 0000  ........U.......
00182210: 0500 0000 0600 0000 0700 0000 0800 0000  ................
00182220: 0900 0000 0a00 0000 0b00 0000 0c00 0000  ................
00182230: 0d00 0000 0e00 0000 0f00 0000 1000 0000  ................

# データ領域 : 0x200400 -

# ルートディレクトリのFAT: N=2 -> 0x255  -> 0x0fffffff (0x104954)
#   N=2のデータ     : 0x200400 = (2 - 2) * 0x200 + 0x200400
#   N=0x255のデータ : 0x24aa00 = (0x255 - 2) * 0x200 + 0x200400
00200400: 4b45 524e 454c 3820 494d 4720 1800 5a46  KERNEL8 IMG ..ZF
00200410: 1255 1255 0000 5a46 1255 0300 e41e 0300  .U.U..ZF.U......
00200420: 4143 004f 0050 0059 0049 000f 0012 4e00  AC.O.P.Y.I....N.
00200430: 4700 2e00 6c00 6900 6e00 0000 7500 7800  G...l.i.n...u.x.
00200440: 434f 5059 494e 7e31 4c49 4e20 0000 5a46  COPYIN~1LIN ..ZF
00200450: 1255 1255 0000 5a46 1255 9301 0549 0000  .U.U..ZF.U...I..
00200460: 4263 006f 006d 0000 00ff ff0f 0067 ffff  Bc.o.m.......g..
00200470: ffff ffff ffff ffff ffff 0000 ffff ffff  ................
00200480: 014c 0049 0043 0045 004e 000f 0067 4300  .L.I.C.E.N...gC.
00200490: 4500 2e00 6200 7200 6f00 0000 6100 6400  E...b.r.o...a.d.
002004a0: 4c49 4345 4e43 7e31 4252 4f20 0000 5a46  LICENC~1BRO ..ZF
002004b0: 1255 1255 0000 5a46 1255 b801 3a06 0000  .U.U..ZF.U..:...
002004c0: 414d 0061 006b 0065 0066 000f 00c1 6900  AM.a.k.e.f....i.
002004d0: 6c00 6500 0000 ffff ffff 0000 ffff ffff  l.e.............
002004e0: 4d41 4b45 4649 4c45 2020 2020 0000 5a46  MAKEFILE    ..ZF
002004f0: 1255 1255 0000 5a46 1255 bc01 6303 0000  .U.U..ZF.U..c...
00200500: 422e 0062 0069 006e 0000 000f 0072 ffff  B..b.i.n.....r..
00200510: ffff ffff ffff ffff ffff 0000 ffff ffff  ................
00200520: 0161 0072 006d 0073 0074 000f 0072 7500  .a.r.m.s.t...ru.
00200530: 6200 3800 2d00 7200 7000 0000 6900 3400  b.8.-.r.p...i.4.
00200540: 4152 4d53 5455 7e31 4249 4e20 0000 5a46  ARMSTU~1BIN ..ZF
00200550: 1255 1255 0000 5a46 1255 be01 0004 0000  .U.U..ZF.U......
00200560: 4152 4d53 5455 4238 5320 2020 0800 5a46  ARMSTUB8S   ..ZF
00200570: 1255 1255 0000 5a46 1255 c001 7c16 0000  .U.U..ZF.U..|...
00200580: 424f 4f54 434f 4445 4249 4e20 1800 5a46  BOOTCODEBIN ..ZF
00200590: 1255 1255 0000 5a46 1255 cc01 e8cc 0000  .U.U..ZF.U......
002005a0: 434f 4e46 4947 2020 5458 5420 1800 5a46  CONFIG  TXT ..ZF
002005b0: 1255 1255 0000 5a46 1255 3302 9a00 0000  .U.U..ZF.U3.....
002005c0: 4649 5855 5020 2020 4441 5420 1800 5a46  FIXUP   DAT ..ZF
002005d0: 1255 1255 0000 5a46 1255 3402 8f1c 0000  .U.U..ZF.U4.....
002005e0: 4649 5855 5034 2020 4441 5420 1800 5a46  FIXUP4  DAT ..ZF
002005f0: 1255 1255 0000 5a46 1255 4302 4215 0000  .U.U..ZF.UC.B...

0024aa00: 4649 5855 5034 4344 4441 5420 1800 5a46  FIXUP4CDDAT ..ZF
0024aa10: 1255 1255 0000 5a46 1255 4e02 6f0c 0000  .U.U..ZF.UN.o...
0024aa20: 4649 5855 505f 4344 4441 5420 1800 5a46  FIXUP_CDDAT ..ZF
0024aa30: 1255 1255 0000 5a46 1255 5602 6f0c 0000  .U.U..ZF.UV.o...
0024aa40: 494e 4954 2020 2020 475a 2020 1800 5a46  INIT    GZ  ..ZF
0024aa50: 1255 1255 0000 5a46 1255 5d02 ba01 0000  .U.U..ZF.U].....
0024aa60: 5354 4152 5420 2020 454c 4620 1800 5a46  START   ELF ..ZF
0024aa70: 1255 1255 0000 5a46 1255 5e02 8019 2d00  .U.U..ZF.U^...-.
0024aa80: 5354 4152 5434 2020 454c 4620 1800 5b46  START4  ELF ..[F
0024aa90: 1255 1255 0000 5b46 1255 eb18 a00c 2200  .U.U..[F.U....".
0024aaa0: 5354 4152 5434 4344 454c 4620 1800 5b46  START4CDELF ..[F
0024aab0: 1255 1255 0000 5b46 1255 f229 1c23 0c00  .U.U..[F.U.).#..
0024aac0: 5354 4152 545f 4344 454c 4620 1800 5b46  START_CDELF ..[F
0024aad0: 1255 1255 0000 5b46 1255 0430 1c23 0c00  .U.U..[F.U.0.#..

# ディレクトリエントリ

## 1. kernel8.img

DIR_Name         : 4b45524e454c3820494d47    : KERNEL8 IMG
DIR_Attr         : 0x20                      : ATTR_ARCHIVE (アーカイブ)
DIR_NTRes        : 0x18                      : ボディがすべて小文字 | 拡張子がすべて小文字
DIR_CrtTimeTenth : 00
DIR_CrtTime      : 0x465a
DIR_CrtDate      : 0x5512
DIR_LstAccDate   : 0x5512
DIR_FstClusHI    : 0
DIR_WrtTime      : 0x465a
DIR_WrtDate      : 0x5512
DIR_FstClusLO    : 0x03
DIR_FileSize     : 0x31ee4

# N : 3 -> 4 -> ... -> 0x192  -> 0xfffffff (0x192-2 * 0x200 = 0x32000)
#    N=3のデータ : 0x200600 = (3 - 2) * 0x200 + 0x200400

00200600: 6001 0010 2100 80d2 4200 80d2 6300 80d2  `...!...B...c...
00200610: 091b 80d2 2079 21f8 2079 22f8 2079 23f8  .... y!. y". y#.
00200620: 9f3f 03d5 df3f 03d5 9f20 03d5 4942 38d5  .?...?... ..IB8.
00200630: 2905 7e92 3f21 00f1 2001 0054 2c00 0054  ).~.?!.. ..T,..T
...
002324c0: 20b3 0a00 0000 ffff 28b3 0a00 0000 ffff   .......(.......
002324d0: 30b3 0a00 0000 ffff 0000 0000 0100 0000  0...............
002324e0: 0000 0000

## 2. COPYING.linux

# LNFエントリ

00200420: 4143 004f 0050 0059 0049 000f 0012 4e00  AC.O.P.Y.I....N.
00200430: 4700 2e00 6c00 6900 6e00 0000 7500 7800  G...l.i.n...u.x.

LDIR_Ord         : 0x41                           : 0x01 + 0x40
LDIR_Name1       : 4300 4f00 5000 5900 4900       : COPYI
LDIR_Attr        : 0x0f                           : ATTR_LONG_NAME
LDIR_Type        : 00
LDIR_Chksum      : 12
LDIR_Name2       : 4e00 4700 2e00 6c00 6900 6e00  : NG.lin
LDIR_FstClusLO   : 0000
LDIR_Name3       : 7500 7800                      : ux

# SFNエントリ

00200440: 434f 5059 494e 7e31 4c49 4e20 0000 5a46  COPYIN~1LIN ..ZF
00200450: 1255 1255 0000 5a46 1255 9301 0549 0000  .U.U..ZF.U...I..

DIR_Name         : 434f5059494e7e314c494e   : COPYIN~1LIN
DIR_Attr         : 0x20        : ATTR_ARCHIVE (アーカイブ)
DIR_NTRes        : 0
DIR_CrtTimeTenth : 0
DIR_CrtTime      : 0x465a
DIR_CrtDate      : 0x5512
DIR_LstAccDate   : 0x5512
DIR_FstClusHI    : 0
DIR_WrtTime      : 0x465a
DIR_WrtDate      : 0x5512
DIR_FstClusLO    : 0x0193
DIR_FileSize     : 0x4905

## 3. LICENCE.broadcom

00200460: 4263 006f 006d 0000 00ff ff0f 0067 ffff  Bc.o.m.......g..
00200470: ffff ffff ffff ffff ffff 0000 ffff ffff  ................
00200480: 014c 0049 0043 0045 004e 000f 0067 4300  .L.I.C.E.N...gC.
00200490: 4500 2e00 6200 7200 6f00 0000 6100 6400  E...b.r.o...a.d.
002004a0: 4c49 4345 4e43 7e31 4252 4f20 0000 5a46  LICENC~1BRO ..ZF
002004b0: 1255 1255 0000 5a46 1255 b801 3a06 0000  .U.U..ZF.U..:...

LDIR_Ord         : 0x42                           : 0x02 + 0x40
LDIR_Name1       : 6300 6f00 6d00 0000 ffff       : com
LDIR_Attr        : 0x0f                           : ATTR_LONG_NAME
LDIR_Type        : 00
LDIR_Chksum      : 67
LDIR_Name2       : ffff ffff ffff ffff ffff       :
LDIR_FstClusLO   : 0000
LDIR_Name3       : ffff ffff                      :

LDIR_Ord         : 0x01                           : 0x01
LDIR_Name1       : 4c00 4900 4300 4500 4e00       : LICEN
LDIR_Attr        : 0x0f                           : ATTR_LONG_NAME
LDIR_Type        : 00
LDIR_Chksum      : 67
LDIR_Name2       : 4300 4500 2e00 6200 7200 6f00  : CE.bro
LDIR_FstClusLO   : 0000
LDIR_Name3       : 6100 6400                      : ad

DIR_Name         : 4c49 4345 4e43 7e31 4252 4f  : LICENC~1BRO
DIR_Attr         : 0x20        : ATTR_ARCHIVE (アーカイブ)
DIR_NTRes        : 0
DIR_CrtTimeTenth : 0
DIR_CrtTime      : 0x465a
DIR_CrtDate      : 0x5512
DIR_LstAccDate   : 0x5512
DIR_FstClusHI    : 0
DIR_WrtTime      : 0x465a
DIR_WrtDate      : 0x5512
DIR_FstClusLO    : 0x01b8
DIR_FileSize     : 0x0638
```

## mtoolsの導入

- 思っているものとは違った
- symlinkで起動に時間がかかるので削除

```
$ mkdir ~/gnu/mtools
$ wget http://ftp.gnu.org/gnu/mtools/mtools-4.0.40.tar.bz2
$ tar xf mtools-4.0.40.tar.bz2
$ cd mtools-4.0.40
$ CC=~/musl/bin/musl-gcc ./configure --host=aarch64-elf --prefix=/Users/dspace/gnu/mtools CFLAGS="-O3 -fpie"
$ make LDFLAGS="-pie"           // LDFLAGSを付けないとEXEC (Executable file)として作成され、filecloseで
$ make install                  // ref < 1でpanicとなる
$ ls ~/gnu/mtools
bin  share
$ ls ~/gnu/mtools/bin
amuFormat.sh  mcd     mdeltree  mkmanifest  mpartition  mtools      tgz
lz            mcheck  mdir      mlabel      mrd         mtoolstest  uz
mattrib       mcomp   mdu       mmd         mren        mtype
mbadblocks    mcopy   mformat   mmount      mshortname  mxtar
mcat          mdel    minfo     mmove       mshowfat    mzip
$ cp ~/gnu/mtools/bin/mtools XV6/usr/bin
$ vi XV6/usr/inc/files.h
usrbins[]にusr/bin/mtoolsを追加
$ rm -f obj/fs.img
$ make
$ make qemu
...
$ mtools
[1]fileopen: cant namei /etc/mtools.conf
[0]fileopen: cant namei /etc/default/mtools.conf
[3]fileopen: cant namei /Users/dspace/gnu/mtools/etc/mtools.conf
[0]fileopen: cant namei /etc/mtools
[3]fileopen: cant namei /etc/default/mtools
[1]fileopen: cant namei /home/zuki/.mtoolsrc
Supported commands:
mattrib, mbadblocks, mcat, mcd, mcopy, mdel, mdeltree, mdir
mdoctorfat, mdu, mformat, minfo, mlabel, mmd, mmount, mpartition
mrd, mread, mmove, mren, mshowfat, mshortname, mtoolstest, mtype
mwrite, mzip
$ mdir /
[2]fileopen: cant namei /etc/mtools.conf
[0]fileopen: cant namei /etc/default/mtools.conf
[2]fileopen: cant namei /Users/dspace/gnu/mtools/etc/mtools.conf
[3]fileopen: cant namei /etc/mtools
[3]fileopen: cant namei /etc/default/mtools
[3]fileopen: cant namei /home/zuki/.mtoolsrc
Drive 'A:' not supported
Cannot initialize 'A:'
$
```
