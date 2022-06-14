# Macで開発

- READMEにあるMacで開発する手順にしたがったところ、問題なく実行できた
- bigtestも実機の1/2速くらいで動く

```
$ brew install zstd
$ brew tap messense/macos-cross-toolchains
$ brew install aarch64-unknown-linux-gnu
$ vi config.mk      // CROSS := aarch64-unknown-linux-gnu- に変更
$ git submodule update --init --recursive
$ (cd libc && export CROSS_COMPILE=aarch64-unknown-linux-gnu- && ./configure --target=aarch64)
$ brew install mtools
$ brew install util-linux
$ echo 'export PATH="/usr/local/opt/util-linux/bin:$PATH"' >> /Users/dspace/.bash_profile
$ echo 'export PATH="/usr/local/opt/util-linux/sbin:$PATH"' >> /Users/dspace/.bash_profile
$ which sfdisk
/usr/local/opt/util-linux/sbin/sfdisk
$ make qemu
...
qemu-system-aarch64 -M raspi3b -nographic -serial null -serial mon:stdio -drive file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
[0]free_range: 0xffff0000000a5000 ~ 0xffff00003c000000, 245595 pages
[3]main: cpu 3 init finished
[1]main: cpu 1 init finished
[2]main: cpu 2 init finished
[0]main: cpu 0 init finished
[1]emmc_card_init: poweron
[1]emmc_card_reset: control0: 0x0, control1: 0x0, control2: 0x0
[1]emmc_card_reset: status: 0x1ff0000
[1]emmc_get_base_clock: base clock rate is 50000000 Hz
[1]emmc_get_clock_divider: base_clock: 50000000, target_rate: 400000, divisor: 0x40, actual_clock: 390625, ret: 0x4000
[1]emmc_issue_command_int: rrror occured whilst waiting for command complete interrupt
[1]emmc_card_reset: OCR: 0xffff, 1.8v support: 0, SDHC support: 0
[1]emmc_get_clock_divider: base_clock: 50000000, target_rate: 25000000, divisor: 0x1, actual_clock: 25000000, ret: 0x100
[1]emmc_card_reset: card CID: 0xaa5859, 0x51454d55, 0x2101dead, 0xbeef0062
[1]emmc_card_reset: RCA: 0x4567
[1]emmc_card_reset: SCR: version 2.00, bus_widths 0x5
[1]emmc_card_reset: found valid version 2.00 SD card
[1]dev_init: LBA of 1st block 0x20800, 0x1f800 blocks totally
[1]iinit: sb: size 20000 nblocks 19937 ninodes 200 nlog 30 logstart 2 inodestart 32 bmapstart 58
init: starting sh
sh: argv[0] = 'sh'
sh: testenv = 'FROM_INIT'
$ ls
.              4000 1 512
..             4000 1 512
cat            8000 2 39104
init           8000 3 23216
bigtest        8000 4 39128
echo           8000 5 39328
mkfs           8000 6 45152
sh             8000 7 52712
utest          8000 8 16752
ls             8000 9 39552
console        0 10 0
$ bigtest
.....................................................................................................................................................................
wrote 16523 sectors
read; ok
done; ok
$
```
