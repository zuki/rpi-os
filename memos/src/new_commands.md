# 各種コマンドの追加

## 1. `file`コマンド

### ビルドとxv6への導入

```
$ wget https://astron.com/pub/file/file-5.40.tar.gz
$ tar xf file-5.40.tar.gz
$ cd file-5.40
$ CC=~/musl/bin/musl-gcc ./configure --host=aarch64-elf CFLAGS="-std=gnu99 -O3 -MMD -MP -fpie -Isrc" LDFLAGS="-pie"
$ make

$ cd XV6
$ mkdir -p usr/local/bin
$ mkdir -p usr/local/share/misc
$ cp $FILE/src/file usr/local/bin
$ cp $FILE/magic/magic.mgc /usr/local/share/misc
$ vi usr/inc/files.h
$ vi usr/src/mkfs/main.c
$ rm -rf obj
$ make
```

### 実行

```
$ file /bin/ls
CurrentEL: 0x1
DAIF: Debug(1) SError(1) IRQ(1) FIQ(1)
SPSel: 0x1
SPSR_EL1: 0x40000000
SP: 0xffff0000389b1e00
SP_EL0: 0xffffffffec10
ELR_EL1: 0xc00000105c90, EC: 0x0, ISS: 0x0.
FAR_EL1: 0x0
Unexpected syscall #67 (unknown)                // sys_pread64が未実装
```

### sys_pread64()を実装

- 正常に動くが、magicファイルがキャッシュされず毎回7MB近いファイルを読むので非常に時間がかかる
- filereadにpage_cacheを使うようにしたらloginが動かなかった。とりあえずTODO。

```
$ file /bin/ls
/bin/ls: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, not stripped
$ file /usr/bin/dash
/usr/bin/dash: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped
$ file /bin/mysh
/bin/mysh: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, not stripped
$
```

### 変更履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	modified:   inc/file.h
	modified:   inc/syscall1.h
	modified:   kern/file.c
	modified:   kern/syscall.c
	modified:   kern/sysfile.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/new_commands.md
	modified:   usr/inc/files.h
	new file:   usr/local/share/misc/magic.mgc
	modified:   usr/src/mkfs/main.c
```
