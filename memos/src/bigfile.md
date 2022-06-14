# 02: BigFile機能を実装

- 以下のファイルを修正または追加

  ```
  modified:   inc/file.h
	modified:   inc/fs.h
	modified:   inc/types.h
	modified:   kern/file.c
	modified:   kern/fs.c
	modified:   kern/syscall.c
	modified:   kern/sysfile.c
	new file:   usr/src/bigtest/main.c
	modified:   usr/src/mkfs/main.c
  ```

- 補足説明
  - MITのxv6-publicからuser/big.cをコピー
  - システムコール(sys_write, sys_unllinkat)を追加
  - ssize_tの定義を修正

## big_file対応前のbigtest

```
$ bigtest
.
wrote 140 sectors
read; ok
done; ok
```

## big_file対応後のbigtest

- qemuではおそすぎて終わらない。実機では1ドット（100セクタ）1秒くらいで実行できる。
- 以下は実機で実行した結果

```
$ bigtest
................................................................................
wrote 16523 sectors
read; ok
done; ok
```
