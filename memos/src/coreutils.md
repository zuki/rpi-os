# 12: coreutilsを導入

## 入手と展開

```
$ wget https://ftp.jaist.ac.jp/pub/GNU/coreutils/coreutils-8.32.tar.xz
$ tar Jxf coreutils-8.32.tar.xz
```

## src以外はgitから無視する

```
$ vi .gitginore
/coreutils-8.32/*
!coreutils-8.32/src
```
