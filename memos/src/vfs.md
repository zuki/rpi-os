# VFSをサポート

[caiolima/xv6-public](https://github.com/caiolima/xv6-public)を使用

## 問題1 `/bin/init`が読み込めない

- execve()でget_file("/bin/init")するとv6_inode.addrが消える

```
[1]execve: path='/bin/init', proc: uid=0, gid=0, ip: inum=22, mode=0x81ed, uid=0, gid=0
[1]readi: dev: 1, off: 0x0, blksize: 0xbf
[1]readi: dev: 1, off: 0x1000, blksize: 0xc0
[1]readi: dev: 1, off: 0x2000, blksize: 0xc1
[1]readi: dev: 1, off: 0x3000, blksize: 0xc2

$1 = {
  dev = 0x1,
  inum = 0x16,
  ref = 0x1,
  lock = {
    locked = 0x1,
    lk = {
      locked = 0x0,
      name = 0xffff0000000a94c8
    },
    pid = 0x1,
    name = 0xffff0000000abdf0
  },
  valid = 0x1,
  fs_t = 0xffff0000000b2e78,
  iops = 0xffff0000000b2e08,
  i_private = 0xffff00003b3feeb0,
  type = 0x2,
  major = 0x0,
  minor = 0x0,
  nlink = 0x1,
  size = 0x5808,
  mode = 0x81ed,
  uid = 0x0,
  gid = 0x0,
  atime = {
    tv_sec = 0x62ecc579,
    tv_nsec = 0x2169ecb0
  },
  mtime = {
    tv_sec = 0x62ecc579,
    tv_nsec = 0x2169ecb0
  },
  ctime = {
    tv_sec = 0x62ecc579,
    tv_nsec = 0x2169ecb0
  }
}
(gdb) p/x *(struct v6_inode *)ip->i_private     // get_file()内でnameiしたときにはある
$2 = {
  flag = 0x0,
  addrs = {0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0}
}


(gdb) p/x *(struct v6_inode *)f->ip->i_private  // get_file()を抜けると消える
$2 = {
  flag = 0x0,
  addrs = {0x0, repeat }
}
```

### 問題1 解決

- get_file()の最後でiunlockput()していたため
- put()するとip->ref--してref=0でcleanup()するので0クリアされていた

## 問題2 `/dev/tty`がcant namei

```
[0]fileopen: cant namei /dev/tty
```

- これもv6_inodeのaddrがセットされていないことが原因

```
(gdb) p ip->inum
$6 = 3                  // /dev
(gdb) s
v6_ilock (ip=0xffff00000027df28 <icache+176>) at kern/v6.c:552
552	    if (ip == 0 || ip->ref < 1)
(gdb) n
555	    struct v6_superblock *v6sb = sb[ip->dev].fs_info;
(gdb)
556	    struct v6_inode *v6ip = (struct v6_inode *)ip->i_private;
(gdb)
558	    acquiresleep(&ip->lock);
(gdb)
559	    if (ip->valid == 0) {
(gdb) p ip->valid
$7 = 1												// valid=1だが
(gdb) p/x *(struct v6_inode *)ip->i_private
$8 = {
  flag = 0x0,
  addrs = {0x0 <repeats 13 times>}					// v6_inodeは設定されていない
}
```

- ip->inum=3のvalidを出力

```
[2]execve: path='/bin/init', proc: uid=0, gid=0, ip: inum=22, mode=0x81ed, uid=0, gid=0
[2]v6_ilock: ip->inum=3: valid=1    // はじめからvalid=1でvalid=0のときがない
[2]fileopen: cant namei /dev/tty
[1]v6_ilock: ip->inum=3: valid=1
[1]v6_ilock: ip->inum=3: valid=1
[3]v6_ilock: ip->inum=3: valid=1
[3]fileopen: cant namei /dev/tty
```

### 問題2 解決

- 該当ipがはじめからvalid=1でdip->addrが設定されないため
- はじめからvalid=1になるのはv6_igetでvalidをゼロクリアしていなかったため

## 実行例

```
dd if=/dev/zero of=obj/boot.img seek=$((128*1024 - 1)) bs=512 count=1
1+0 records in
1+0 records out
512 bytes transferred in 0.000053 secs (9629972 bytes/sec)
# -F specify FAT32
# -c 1 specify one sector per cluster so that we can create a smaller one
mformat -F -c 1 -i obj/boot.img ::
# Copy files into boot partition
mcopy -i obj/boot.img obj/kernel8.img ::kernel8.img;  mcopy -i obj/boot.img boot/COPYING.linux ::COPYING.linux;  mcopy -i obj/boot.img boot/LICENCE.broadcom ::LICENCE.broadcom;  mcopy -i obj/boot.img boot/Makefile ::Makefile;  mcopy -i obj/boot.img boot/armstub8-rpi4.bin ::armstub8-rpi4.bin;  mcopy -i obj/boot.img boot/armstub8.S ::armstub8.S;  mcopy -i obj/boot.img boot/bootcode.bin ::bootcode.bin;  mcopy -i obj/boot.img boot/config.txt ::config.txt;  mcopy -i obj/boot.img boot/fixup.dat ::fixup.dat;  mcopy -i obj/boot.img boot/fixup4.dat ::fixup4.dat;  mcopy -i obj/boot.img boot/fixup4cd.dat ::fixup4cd.dat;  mcopy -i obj/boot.img boot/fixup_cd.dat ::fixup_cd.dat;  mcopy -i obj/boot.img boot/start.elf ::start.elf;  mcopy -i obj/boot.img boot/start4.elf ::start4.elf;  mcopy -i obj/boot.img boot/start4cd.elf ::start4cd.elf;  mcopy -i obj/boot.img boot/start_cd.elf ::start_cd.elf;
dd if=/dev/zero of=obj/sd.img seek=$((2048*1024 - 1)) bs=512 count=1
1+0 records in
1+0 records out
512 bytes transferred in 0.000045 secs (11362347 bytes/sec)
printf "                                                                \
	  2048, $((128*1024*512 / 1024))K, c,\n      \
	  , $((960*1024*512 / 1024))K, L,\n          \
	  1116160, $((980992*512 / 1024))K, L,\n  \
	" | sfdisk obj/sd.img
このディスクを使用しているユーザがいないかどうかを調べています ... OK

ディスク obj/sd.img: 1 GiB, 1073741824 バイト, 2097152 セクタ
単位: セクタ (1 * 512 = 512 バイト)
セクタサイズ (論理 / 物理): 512 バイト / 512 バイト
I/O サイズ (最小 / 推奨): 512 バイト / 512 バイト
ディスクラベルのタイプ: dos
ディスク識別子: 0xc6dd753e

古い状態:

デバイス    起動 開始位置 終了位置 セクタ サイズ Id タイプ
obj/sd.img1          2048   133119 131072    64M  c W95 FAT32 (LBA)
obj/sd.img2        133120  1116159 983040   480M 83 Linux
obj/sd.img3       1116160  2097151 980992   479M 83 Linux

>>> 新しい DOS ディスクラベルを作成しました。識別子は 0xe27f8237 です。
obj/sd.img1: 新しいパーティション 1 をタイプ W95 FAT32 (LBA)、サイズ 64 MiB で作成しました。
パーティション #1 には vfat 署名が書き込まれています。
obj/sd.img2: 新しいパーティション 2 をタイプ Linux、サイズ 480 MiB で作成しました。
obj/sd.img3: 新しいパーティション 3 をタイプ Linux、サイズ 479 MiB で作成しました。
パーティション #3 には ext2 署名が書き込まれています。
obj/sd.img4: 終了。

新しい状態:
ディスクラベルのタイプ: dos
ディスク識別子: 0xe27f8237

デバイス    起動 開始位置 終了位置 セクタ サイズ Id タイプ
obj/sd.img1          2048   133119 131072    64M  c W95 FAT32 (LBA)
obj/sd.img2        133120  1116159 983040   480M 83 Linux
obj/sd.img3       1116160  2097151 980992   479M 83 Linux

パーティション情報が変更されました。
ディスクを同期しています。
dd if=obj/boot.img of=obj/sd.img seek=2048 conv=notrunc
131072+0 records in
131072+0 records out
67108864 bytes transferred in 0.813602 secs (82483655 bytes/sec)
dd if=obj/fs.img of=obj/sd.img seek=133120 conv=notrunc
800000+0 records in
800000+0 records out
409600000 bytes transferred in 4.136025 secs (99032285 bytes/sec)
dd if=obj/ext2.img of=obj/sd.img seek=1116160 conv=notrunc
61440+0 records in
61440+0 records out
31457280 bytes transferred in 0.315496 secs (99707392 bytes/sec)
qemu-system-aarch64 -M raspi3b -nographic -serial null -serial mon:stdio -drive file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
[0]free_range: 0xffff00000035b000 ~ 0xffff00003b400000, 241829 pages
[0]console_init: devsw[1].termios: 0xffff00003b3ff000
[0]rand_init: rand_init ok
[0]init_vfssw: init_vfssw ok
[0]install_rootfs: install_rootfs ok
pagecache_init ok
[3]main: cpu 3 init finished
[1]main: cpu 1 init finished
[0]main: cpu 0 init finished
[2]main: cpu 2 init finished
[3]emmc_card_init: poweron
[3]emmc_issue_command_int: rrror occured whilst waiting for command complete interrupt
[3]emmc_card_reset: found valid version 2.00 SD card
[3]iinit: sb: size 100000 nblocks 99835 ninodes 1024 nlog 126 logstart 2 inodestart 128 bmapstart 161
[3]initlog: init log ok

[3]fileopen: cant namei /etc/issue
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
[0]fileopen: cant namei /etc/profile
[3]fileopen: cant namei /.profile
$ ls -l /bin
total 138
-rwxr-xr-x 1 root root 39816 Aug  6  2022 bigtest
-rwxr-xr-x 1 root root 38568 Aug  6  2022 cat
-rwxr-xr-x 1 root root 48552 Aug  6  2022 date
-rwxr-xr-x 1 root root 39480 Aug  6  2022 echo
-rwxr-xr-x 1 root root 62200 Aug  6  2022 getty
-rwxr-xr-x 1 root root  4064 Aug  6  2022 hello-dyn
-rwxr-xr-x 1 root root 22536 Aug  6  2022 init
-rwxr-xr-x 1 root root 99704 Aug  6  2022 login
-rwxr-xr-x 1 root root 53064 Aug  6  2022 ls
-rwxr-xr-x 1 root root 53504 Aug  6  2022 mkfs
-rwxr-xr-x 1 root root 49376 Aug  6  2022 mmaptest
-rwxr-xr-x 1 root root 74544 Aug  6  2022 mmaptest2
-rwxr-xr-x 1 root root 15520 Aug  6  2022 mount
-rwxr-xr-x 1 root root 54056 Aug  6  2022 mysh
-rwsr-xr-x 1 root root 92032 Aug  6  2022 passwd
-rwxr-xr-x 1 root root 45936 Aug  6  2022 pipetest
-rwxr-xr-x 1 root root 45056 Aug  6  2022 sigtest
-rwxr-xr-x 1 root root 48640 Aug  6  2022 sigtest2
-rwxr-xr-x 1 root root 22104 Aug  6  2022 sigtest3
-rwsr-xr-x 1 root root 93728 Aug  6  2022 su
-rwxr-xr-x 1 root root 47320 Aug  6  2022 timertest
-rwxr-xr-x 1 root root 15488 Aug  6  2022 umount
-rwxr-xr-x 1 root root 17744 Aug  6  2022 utest
$ ls /
bin  dev  etc  home  lib  test.txt  usr
```
