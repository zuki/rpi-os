# BSIZEを4096に変更

## BSIZE = 512の場合の起動情報の抜粋

```
nmeta 581 (boot, super, log blocks 126 inode blocks 257, bitmap blocks 196) blocks 799419 total 800000

dd if=/dev/zero of=obj/sd.img seek=$((2048*1024 - 1)) bs=512 count=1
1+0 records in
1+0 records out
512 bytes transferred in 0.000032 secs (16025997 bytes/sec)
printf "                                                                \
	  2048, $((128*1024*512/1024))K, c,\n      \
	  133120, $((960*1024*512/1024))K, L,\n          \
	" | sfdisk obj/sd.img

1]emmc_card_reset: found valid version 2.00 SD card
[1]emmc_do_read: reading from block 0
[1]dev_init: LBA of 1st block 0x20800, 0xf0000 blocks totally
[1]emmc_do_read: reading from block 133121(=0x20801)
[1]iinit: sb: size 800000 nblocks 799419 ninodes 1024 nlog 126 logstart 2 inodestart 128 bmapstart 385
[1]emmc_do_read: reading from block 133122
[3]emmc_do_read: reading from block 133248
```

## 1. BSIZE = 4096 に変更(inc/fs.h, usr/inc/param.h)

```
qemu-system-aarch64 -M raspi3b -nographic -serial null -serial mon:stdio -drive file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
qemu-system-aarch64: Invalid SD card size: 3.12 GiB
SD card size has to be a power of 2, e.g. 4 GiB.
You can resize disk images with 'qemu-img resize <imagefile> <new-size>'
(note that this will lose data if you make the image smaller than it currently is).
```

- fs.imgのサイズがこれまでの`409,600,000`から`3,276,800,000`と8倍になったため

## 2. FSSIZE = 100000 に変更

- super blockが読めない
- blocknoが正しい番号の8倍

```
nmeta 165 (boot, super, log blocks 126 inode blocks 33, bitmap blocks 4) blocks 99835 total 100000

dd if=/dev/zero of=obj/sd.img seek=$((2048*1024 - 1)) bs=512 count=1
1+0 records in
1+0 records out
512 bytes transferred in 0.000031 secs (16519105 bytes/sec)
printf "                                                                \
	  2048, $((128*1024*512/1024))K, c,\n      \
	  133120, $((960*1024*512/1024))K, L,\n          \
	" | sfdisk obj/sd.img

[2]emmc_card_reset: found valid version 2.00 SD card
[2]emmc_do_read: reading from block 0
[2]dev_init: LBA of 1st block 0x20800, 0xf0000 blocks totally
[2]emmc_do_read: reading from block 1064968(=0x104008)          // 0x20801 * 8
[2]iinit: sb: size 0 nblocks 0 ninodes 0 nlog 0 logstart 0 inodestart 0 bmapstart 0
[2]emmc_do_read: reading from block 1064960
ilock: no typekern/console.c:347: kernel panic at cpu 2.
QEMU: Terminated
```

## 3. devc.c#dev_start()を修正

```
$ git diff kern/dev.c
diff --git a/kern/dev.c b/kern/dev.c
index 77d0957..63b95a1 100644
--- a/kern/dev.c
+++ b/kern/dev.c
@@ -82,9 +82,9 @@ dev_start()
         struct buf *b =
             container_of(list_front(&devque), struct buf, dlink);
         assert(b->blockno < nblocks);
-        uint32_t bno = b->blockno + first_bno;
-
-        emmc_seek(&card, bno * BSIZE);
+        uint32_t bno = b->blockno * 8 + first_bno;
+        debug("b->block: 0x%x, first: 0x%x, bno: 0x%x, seek: 0x%llx", b->blockno, first_bno, bno, bno * BSIZE / 8);
+        emmc_seek(&card, bno * SD_BLOCK_SIZE);
         if (b->flags & B_DIRTY) {
             assert(emmc_write(&card, b->data, BSIZE) == BSIZE);
         } else {
```

### 実行

- 明らかに早くなっている

```
[1]emmc_card_reset: found valid version 2.00 SD card
[1]dev_init: LBA of 1st block 0x20800, 0xf0000 blocks totally
[1]iinit: sb: size 100000 nblocks 99835 ninodes 1024 nlog 126 logstart 2 inodestart 128 bmapstart 161

[3]fileopen: cant namei /etc/issue
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
[2]fileopen: cant namei /etc/profile
[1]fileopen: cant namei /.profile
$ ls /
bin  dev  etc  home  lib  test.txt  usr
$ ls -l /etc/
total 2
-rw-r--r-- 1 root root 188 Jul 29  2022 group
-rw-r--r-- 1 root root 100 Jul 29  2022 inittab
-rw-r--r-- 1 root root 102 Jul 29  2022 passwd
$ ls -li /lib
total 115
167 lrwxrwxrwx 1 root root     12 Jun 21 10:31 ld-musl-aarch64.so.1 -> /lib/libc.so
166 -rwxr-xr-x 1 root root 933056 Jul 29  2022 libc.so
```

## パーティションテーブルを見る

```
                                            1st sec    total sec
1: 00 202100 0c 490108 0008 0000 0000 0200     0x800   0x20000          64 MB
2: 00 490208 83 793445 0008 0200 0000 0f00   0x20800   0xf0000         415 MB
3: 00 000000 00 000000 0000 0000 0000 0000
4: 00 000000 00 000000 0000 0000 0000 0000
```

## 更新履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	modified:   inc/emmc.h
	modified:   inc/fs.h
	modified:   kern/dev.c
	modified:   kern/emmc.c
	new file:   memos/src/4Kblock.md
	modified:   memos/src/SUMMARY.md
	modified:   tools/dump.c
	modified:   usr/inc/param.h
	modified:   usr/src/mkfs/main.c
```
