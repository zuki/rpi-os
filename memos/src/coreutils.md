# 12: coreutilsを導入

## 入手と展開

```
$ wget https://ftp.jaist.ac.jp/pub/GNU/coreutils/coreutils-8.32.tar.xz
$ tar Jxf coreutils-8.32.tar.xz
```

## src以外はgitから無視する

```
$ vi .gitginore
coreutils-8.32/*
!coreutils-8.32/src
coreutils-8.32/src/*
!coreutils-8.32/src/*.[ch]
coreutils-8.32/src/coreutils.h
coreutils-8.32/src/version.[ch]
```

## srcを一部変更

```
$ git diff
diff --git a/coreutils-8.32/src/ls.c b/coreutils-8.32/src/ls.c
index 24b9832..dbd48c2 100644
--- a/coreutils-8.32/src/ls.c
+++ b/coreutils-8.32/src/ls.c
@@ -3023,7 +3023,7 @@ print_dir (char const *name, char const *realname, bool command_line_arg)
         {
           /* If readdir finds no directory entries at all, not even "." or
              "..", then double check that the directory exists.  */
-          if (syscall (SYS_getdents, dirfd (dirp), NULL, 0) == -1
+          if (syscall (SYS_getdents64, dirfd (dirp), NULL, 0) == -1
               && errno != EINVAL)
             {
               /* We exclude EINVAL as that pertains to buffer handling,
```

## configureとmake

```
$ cd coreutils-8.32
$ CC=$HOME/musl/bin/musl-gcc ./configure --host=aarch64-elf CFLAGS="-std=gnu99 -O3 -MMD -MP -static -fno-plt -fno-pic -fpie -z max-page-size=4096"
$ make
$ file src/ls
src/ls: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, with debug_info, not stripped
```

## xv6に組み込む

- `/usr/bin`に置く
- Makefileを使わず、`mkfs`コマンドで直接コピーする


### `usr/inc/usrbins.h`を作成

- findコマンドのオプションがLinuxとは異なる（BSD由来だから）ことに注意

```
$ find . -type f -perm +0111 -not -name 'make-prime-list' -not -name '*.so' > ../../usr/bin/core.txt
$ vi usr/inc/usrbins.h
```

### コマンドをコピー

```
$ find . -type f -perm +0111 -not -name 'make-prime-list' -not -name '*.so' -exec cp {} ../../usr/bin \;
```
