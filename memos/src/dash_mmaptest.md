# 13.1 mmaptestでエラー発生

```
# mmaptest
mmap_testスタート
- open mmap.dur (fd=3) READ ONLY
[1] test mmap f
- mmap fd=3 -> 2ページ分PROT_READ MAP_PRIVATEでマップ: addr=0x600000001000
- munmmap 2PAGE: addr=0x600000001000
[1] OK
[2] test mmap private
- mmap fd=3 -> 2ページ分をPAGE PROT_READ/WRITE, MAP_PRIVATEでマップ: addr=0x600000001000
- closed 3
- write 2PAGE = Z
- p[1]=Z
- munmmap 2PAGE: addr=0x600000001000
[2] OK
[3] test mmap read-only
- open mmap.dur (3) RDONALY
[1]sys_mmap: file is not writable
- mmap 3 -> 3PAGE PROT_READ/WRITE MAP_SHARED: addr=0xffffffffffffffff
- close 3
[3] OK
[4] test mmap read/write
- open mmap.dur (3) RDWR
- mmap 3 -> 3PAGE PROT_READ|WRITE MAP_SHARED: addr=0x600000001000
- close 3
- print 2PAGE = Z
- munmmap 2PAGE: addr=0x600000001000
[4] OK
[5] test mmap dirty
- open mmap.dur (3) RDWR
- close 3
[5] OK
[6] test not-mapped unmap
- munmmap PAGE: addr=0x600000003000
[6] OK
[7] test mmap two files
- open mmap1 (3) RDWR/CREAT
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000001000
- close 3
- unlink mmap1
- open mmap2 (3) RDWR/CREAT			// ここでストール
```

### logが満杯か?

-- mmaptest内のprintf()で呼び出されるsys_writev()でストール
-- test 1-6、test 7とforkテストの2つに分けて実行するとどちらも成功

```
# mmaptest
mmap_testスタート
- open mmap.dur (fd=3) READ ONLY
[1] test mmap f
- mmap fd=3 -> 2ページ分PROT_READ MAP_PRIVATEでマップ: addr=0x600000001000
- munmmap 2PAGE: addr=0x600000001000
[1] OK
[2] test mmap private
- mmap fd=3 -> 2ページ分をPAGE PROT_READ/WRITE, MAP_PRIVATEでマップ: addr=0x600000001000
- closed 3
- write 2PAGE = Z
- p[1]=Z
- munmmap 2PAGE: addr=0x600000001000
[2] OK
[3] test mmap read-only
- open mmap.dur (3) RDONALY
[3]sys_mmap: file is not writable
- mmap 3 -> 3PAGE PROT_READ/WRITE MAP_SHARED: addr=0xffffffffffffffff
- close 3
[3] OK
[4] test mmap read/write
- open mmap.dur (3) RDWR
- mmap 3 -> 3PAGE PROT_READ|WRITE MAP_SHARED: addr=0x600000001000
- close 3
- print 2PAGE = Z
- munmmap 2PAGE: addr=0x600000001000
[4] OK
[5] test mmap dirty
- open mmap.dur (3) RDWR
- close 3
[5] OK
[6] test not-mapped unmap
- munmmap PAGE: addr=0x600000003000
[6] OK
mmap_test: Total: 6, OK: 6, NG: 0
mmaptest: all tests succeeded

# mmaptest
mmap_testスタート
[7] test mmap two files
- open mmap1 (3) RDWR/CREAT
- write 3: 12345
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000001000
- close 3
- unlink mmap1
- open mmap2 (3) RDWR/CREAT
- write 3: 67890
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000002000
- close 3
- unlink mmap2
- munmap PAGE: addr=0x600000001000
- munmap PAGE: addr=0x600000002000
[7] OK
mmap_test: Total: 1, OK: 1, NG: 0

fork_test starting
p1[PGSIZE]=A
p2[PGSIZE]=A
fork_test OK
mmaptest: all tests succeeded
```

## mmaptest2

```
# mmaptest2
[F-01] 不正なfdを指定した場合のテスト
 error: Bad file descriptor
 error: Bad file descriptor
 error: Bad file descriptor
[F-01] ok

[F-02] 不正なフラグを指定した場合のテスト
[3]sys_mmap: invalid flags: 0x0
 error: Invalid argument
[F-02] ok

[F-03] readonlyファイルにPROT_WRITEな共有マッピングをした場合のテスト
[0]sys_mmap: file is not writable
 error: Permission denied
[F-03] ok

[F-04] readonlyファイルにPROT_READのみの共有マッピングをした場合のテスト
[F-04] ok

[F-05] PROT指定の異なるプライベートマッピンのテスト
[F-05] ok

[F-06] MMAPTOPを超えるサイズをマッピングした場合のテスト
[2]get_page: get_page readi failed: n=-1, offset=4096, size=4096
copy_page: get_page failed
[2]map_file_page: map_pagecache_page: copy_page failed
 error: Out of memory
[F-06] ok

[F-07] 連続したマッピングを行うテスト			// ここでストール（mmaptestと同じ現象か?)
```

### F-07からテストスタート

```
# mmaptest2

[F-07] 連続したマッピングを行うテスト
[F-07] ok

[F-08] 空のファイルを共有マッピングした場合のテスト
[F-08] ok

[F-09] ファイルが背後にあるプライベートマッピング
[F-09] ok

[F-10] ファイルが背後にある共有マッピングのテスト
[F-10] ok

[F-11] file backed mapping pagecache coherency test
[F-11] ok

[F-12] file backed private mapping with fork test
[F-12] ok

[F-13] file backed shared mapping with fork test
[F-13] failed at strcmp parent: ret2[0]=a, buf[0]=a

[F-14] オフセットを指定したプライベートマッピングのテスト
[F-14] ok

[F-15] file backed valid provided address test
[F-15] ok

[F-16] file backed invalid provided address test
[F-16] failed

[F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合
[F-17] failed: at second mmap

[F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト
ret=0x600000001000
ret2=0x600000003000
not mapped
kern/console.c:283: kernel panic at cpu 2.

[F-19] ２つのマッピングの間に不可能なアドレスを指定した場合
not mapped
kern/console.c:283: kernel panic at cpu 1.
```

### anonymous_test()

```
# mmaptest2

[A-01] anonymous private mapping test
p[0]=0, p[2499]=2499
[A-01] ok

[A-02] anonymous shared mapping test						// 結果が出ていない

[A-03] anonymous private mapping with fork test
[A-03] ok

[A-04] anonymous shared mapping with multiple forks test
[3]exit: exit: pid 12, name , err 1
[0]exit: exit: pid 11, name , err 1
[1]exit: exit: pid 10, name , err 1
[A-04] failed at strcmp fork 1 parent

[A-05] anonymous private & shared mapping together with fork test
[0]exit: exit: pid 13, name , err 1
[A-05] failed at strcmp share

[A-06] anonymous missing flags test
[3]sys_mmap: invalid flags: 0x20
[A-06] ok

[A-07] anonymous exceed mapping count test
[A-07] ok

[A-08] anonymous exceed mapping size test			// ここでストール.[A-08]は単独でもストール
```

### [A-09]から実行

```
[A-09] anonymous zero size mapping test
[A-09] failed

[A-10] anonymous valid provided address test
[A-10] ok

[A-11] anonymous invalid provided address test
[A-11] failed at mmap

[A-12] anonymous overlapping provided address test
[A-12] failed at second mmap

[A-13] anonymous intermediate provided address test
not mapped
kern/console.c:283: kernel panic at cpu 2.
```

### other_test

```
# mmaptest2

[O-01] munmap only partial size test
[O-01] ok

[O-02] write on read only mapping test
[O-02] ok

[O-03] none permission on mapping test
[O-03] ok

[O-04] mmap valid address map fixed flag test
[O-04] ok

[O-05] mmap invalid address map fixed flag test
[O-05] failed at mmap 1
```

###  7/4

- MAP_FIXEDで指定したアドレスが存在する場合はエラーに変更
- MAP_SHAREDの場合は、free_mmap_list()しない
- sys_msyncを実装
- MAXOPBLOCKSを42に変更
- mmaptestはF-19がregression

```
# mmaptest2
[F-01] 不正なfdを指定した場合のテスト
 error: Bad file descriptor
 error: Bad file descriptor
 error: Bad file descriptor
[F-01] ok

[F-02] 不正なフラグを指定した場合のテスト
[0]sys_mmap: invalid flags: 0x0
 error: Invalid argument
[F-02] ok

[F-03] readonlyファイルにPROT_WRITEな共有マッピングをした場合のテスト
[3]sys_mmap: file is not writable
 error: Permission denied
[F-03] ok

[F-04] readonlyファイルにPROT_READのみの共有マッピングをした場合のテスト
[F-04] ok

[F-05] PROT指定の異なるプライベートマッピンのテスト
[F-05] ok

[F-06] MMAPTOPを超えるサイズをマッピングした場合のテスト
[1]get_page: get_page readi failed: n=-1, offset=4096, size=4096
[1]copy_page: get_page failed
[1]map_file_page: map_pagecache_page: copy_page failed
 error: Out of memory
[F-06] ok

[F-07] 連続したマッピングを行うテスト
[0]uvm_map: remap: p=0x600000001000, *pte=0x3bb3c647

 error: Invalid argument
[F-07] failed at 1 total mappings

[F-08] 空のファイルを共有マッピングした場合のテスト
[F-08] ok

[F-09] ファイルが背後にあるプライベートマッピング
```

```
[F-09] ファイルが背後にあるプライベートマッピング
[F-09] ok

[F-10] ファイルが背後にある共有マッピングのテスト
[F-10] ok

[F-11] file backed mapping pagecache coherency test
[F-11] ok

[F-12] file backed private mapping with fork test
[F-12] ok

[F-13] file backed shared mapping with fork test
[F-13] failed at strcmp parent: ret2[0]=a, buf[0]=a

[F-14] オフセットを指定したプライベートマッピングのテスト
[F-14] ok

[F-15] file backed valid provided address test
[F-15] ok

[F-16] file backed invalid provided address test
[F-16] ok

[F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合
[F-17] ok

[F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト
[F-18] ok

[F-19] ２つのマッピングの間に不可能なアドレスを指定した場合
ret =0x600000001000
ret2=0x600000003000
[2]get_page: get_page readi failed: n=-1, offset=8192, size=4096
[2]copy_page: get_page failed
[2]map_file_page: map_pagecache_page: copy_page failed
ret3=0xffffffffffffffff
[F-19] failed at third mmap

[F-20] 共有マッピングでファイル容量より大きなサイズを指定した場合
[F-20] ok

[F-21] write onlyファイルへのREAD/WRITE共有マッピングのテスト
[0]sys_mmap: file is not readable
[F-21] ok
```

```
# mmaptest2

[A-01] anonymous private mapping test
p[0]=0, p[2499]=2499
[A-01] ok

[A-02] anonymous shared mapping test						// MAP_ANON + MAP_SHARE + fork()が絡むとエラー
[A-02] p1[1]: 0 != 1

[A-03] anonymous private mapping with fork test
[A-03] ok

[A-04] anonymous shared mapping with multiple forks test
[2]exit: exit: pid 12, name , err 1
[3]exit: exit: pid 11, name , err 1
[3]exit: exit: pid 10, name , err 1
[A-04] failed at strcmp fork 1 parent

[A-05] anonymous private & shared mapping together with fork test
[1]exit: exit: pid 13, name , err 1
[A-05] failed at strcmp share

[A-06] anonymous missing flags test
[0]sys_mmap: invalid flags: 0x20
[A-06] ok

[A-07] anonymous exceed mapping count test
[A-07] ok

[A-08] anonymous exceed mapping size test
[A-08] ok

[A-09] anonymous zero size mapping test
[0]sys_mmap: invalid length: 0 or offset: 0
[A-09] ok

[A-10] anonymous valid provided address test
[A-10] ok

[A-11] anonymous invalid provided address test
[A-11] ok: not running because of an invalid test

[A-12] anonymous overlapping provided address test
[A-12] ok

[A-13] anonymous intermediate provided address test
[A-13] ok

[A-14] anonymous intermediate provided address not possible test
[A-14] ok

[O-01] munmap only partial size test
[O-01] ok

[O-02] write on read only mapping test
[O-02] ok

[O-03] none permission on mapping test
[O-03] ok

[O-04] mmap valid address map fixed flag test
[O-04] ok

[O-05] mmap invalid address map fixed flag test
[0]mmap: fixed address should be page align: 0x600000000100
[0]mmap: addr is used
[O-05] test ok

file_test:  ok: 0, ng: 0
anon_test:  ok: 10, ng: 3
other_test: ok: 5, ng: 0
```

### free_mmap_listのMAP_SHAREDの処理を追加するとmmaptestの後にlsが実行できなくなる

```
[0]iinit: sb: size 800000 nblocks 799419 ninodes 1024 nlog 126 logstart 2 inodestart 128 bmapstart 385
[2]uvm_alloc: map: addr=0x400000, length=0x1000, pa=0x3bbe0000
[2]uvm_alloc: map: addr=0x401000, length=0x1000, pa=0x3bbdc000
[2]uvm_alloc: map: addr=0x402000, length=0x1000, pa=0x3bbdb000
[2]uvm_alloc: map: addr=0x403000, length=0x1000, pa=0x3bbda000
[2]uvm_alloc: map: addr=0x404000, length=0x1000, pa=0x3bbd9000
[2]uvm_alloc: map: addr=0x405000, length=0x1000, pa=0x3bbd8000
[2]uvm_alloc: map: addr=0x406000, length=0x1000, pa=0x3bbd7000
[2]uvm_alloc: map: addr=0x407000, length=0x1000, pa=0x3bbd6000
[2]uvm_alloc: map: addr=0x408000, length=0x1000, pa=0x3bbd5000
[2]execve: vm_free
[2]vm_free: free pte =0xffff00003bbfe000
[2]vm_free: free pgt3=0xffff00003bbfa000
[2]vm_free: free pgt2=0xffff00003bbfb000
[2]vm_free: free pgt1=0xffff00003bbfc000
[2]vm_free: free dir =0xffff00003bbfd000
[2]vm_stat: va: 0x400000, pa: 0xffff00003bbe0000, pte: 0x3bbe0647, PTE_ADDR(pte): 0x3bbe0000
[2]vm_stat: va: 0x401000, pa: 0xffff00003bbdc000, pte: 0x3bbdc647, PTE_ADDR(pte): 0x3bbdc000
[2]vm_stat: va: 0x402000, pa: 0xffff00003bbdb000, pte: 0x3bbdb647, PTE_ADDR(pte): 0x3bbdb000
[2]vm_stat: va: 0x403000, pa: 0xffff00003bbda000, pte: 0x3bbda647, PTE_ADDR(pte): 0x3bbda000
[2]vm_stat: va: 0x404000, pa: 0xffff00003bbd9000, pte: 0x3bbd9647, PTE_ADDR(pte): 0x3bbd9000
[2]vm_stat: va: 0x405000, pa: 0xffff00003bbd8000, pte: 0x3bbd8647, PTE_ADDR(pte): 0x3bbd8000
[2]vm_stat: va: 0x406000, pa: 0xffff00003bbd7000, pte: 0x3bbd7647, PTE_ADDR(pte): 0x3bbd7000
[2]vm_stat: va: 0x407000, pa: 0xffff00003bbd6000, pte: 0x3bbd6647, PTE_ADDR(pte): 0x3bbd6000
[2]vm_stat: va: 0x408000, pa: 0xffff00003bbd5000, pte: 0x3bbd5647, PTE_ADDR(pte): 0x3bbd5000
[2]vm_stat: va: 0xffffffff6000, pa: 0xffff00003bbd0000, pte: 0x3bbd0647, PTE_ADDR(pte): 0x3bbd0000
[2]vm_stat: va: [0x400000 ~ 0x409000)
[2]vm_stat: va: 0xffffffff7000, pa: 0xffff00003bbcf000, pte: 0x3bbcf647, PTE_ADDR(pte): 0x3bbcf000
[2]vm_stat: va: 0xffffffff8000, pa: 0xffff00003bbce000, pte: 0x3bbce647, PTE_ADDR(pte): 0x3bbce000
[2]vm_stat: va: 0xffffffff9000, pa: 0xffff00003bbcd000, pte: 0x3bbcd647, PTE_ADDR(pte): 0x3bbcd000
[2]vm_stat: va: 0xffffffffa000, pa: 0xffff00003bbcc000, pte: 0x3bbcc647, PTE_ADDR(pte): 0x3bbcc000
[2]vm_stat: va: 0xffffffffb000, pa: 0xffff00003bbcb000, pte: 0x3bbcb647, PTE_ADDR(pte): 0x3bbcb000
[2]vm_stat: va: 0xffffffffc000, pa: 0xffff00003bbca000, pte: 0x3bbca647, PTE_ADDR(pte): 0x3bbca000
[2]vm_stat: va: 0xffffffffd000, pa: 0xffff00003bbc9000, pte: 0x3bbc9647, PTE_ADDR(pte): 0x3bbc9000
[2]vm_stat: va: 0xffffffffe000, pa: 0xffff00003bbc8000, pte: 0x3bbc8647, PTE_ADDR(pte): 0x3bbc8000
[2]vm_stat: va: 0xfffffffff000, pa: 0xffff00003bbd1000, pte: 0x3bbd1647, PTE_ADDR(pte): 0x3bbd1000
[2]vm_stat: va: [0xffffffff6000 ~ 0x1000000000000)
init: starting sh
[3]uvm_copy: map: addr=0x400000, length=0x1000, pa=0x3bbfb000
[3]uvm_copy: map: addr=0x401000, length=0x1000, pa=0x3bbc6000
[3]uvm_copy: map: addr=0x402000, length=0x1000, pa=0x3bbc5000
[3]uvm_copy: map: addr=0x403000, length=0x1000, pa=0x3bbc4000
[3]uvm_copy: map: addr=0x404000, length=0x1000, pa=0x3bbc3000
[3]uvm_copy: map: addr=0x405000, length=0x1000, pa=0x3bbc2000
[3]uvm_copy: map: addr=0x406000, length=0x1000, pa=0x3bbc1000
[3]uvm_copy: map: addr=0x407000, length=0x1000, pa=0x3bbc0000
[3]uvm_copy: map: addr=0x408000, length=0x1000, pa=0x3bbbf000
[3]uvm_copy: map: addr=0xffffffff6000, length=0x1000, pa=0x3bbbe000
[3]uvm_copy: map: addr=0xffffffff7000, length=0x1000, pa=0x3bbba000
[3]uvm_copy: map: addr=0xffffffff8000, length=0x1000, pa=0x3bbb9000
[3]uvm_copy: map: addr=0xffffffff9000, length=0x1000, pa=0x3bbb8000
[3]uvm_copy: map: addr=0xffffffffa000, length=0x1000, pa=0x3bbb7000
[3]uvm_copy: map: addr=0xffffffffb000, length=0x1000, pa=0x3bbb6000
[3]uvm_copy: map: addr=0xffffffffc000, length=0x1000, pa=0x3bbb5000
[3]uvm_copy: map: addr=0xffffffffd000, length=0x1000, pa=0x3bbb4000
[3]uvm_copy: map: addr=0xffffffffe000, length=0x1000, pa=0x3bbb3000
[3]uvm_copy: map: addr=0xfffffffff000, length=0x1000, pa=0x3bbb2000
[0]uvm_alloc: map: addr=0x400000, length=0x1000, pa=0x3bbb0000
[0]uvm_alloc: map: addr=0x401000, length=0x1000, pa=0x3bbac000
[0]uvm_alloc: map: addr=0x402000, length=0x1000, pa=0x3bbab000
[0]uvm_alloc: map: addr=0x403000, length=0x1000, pa=0x3bbaa000
[0]uvm_alloc: map: addr=0x404000, length=0x1000, pa=0x3bba9000
[0]uvm_alloc: map: addr=0x405000, length=0x1000, pa=0x3bba8000
[0]uvm_alloc: map: addr=0x406000, length=0x1000, pa=0x3bba7000
[0]uvm_alloc: map: addr=0x407000, length=0x1000, pa=0x3bba6000
[0]uvm_alloc: map: addr=0x408000, length=0x1000, pa=0x3bba5000
[0]uvm_alloc: map: addr=0x409000, length=0x1000, pa=0x3bba4000
[0]uvm_alloc: map: addr=0x40a000, length=0x1000, pa=0x3bba3000
[0]uvm_alloc: map: addr=0x40b000, length=0x1000, pa=0x3bba2000
[0]uvm_alloc: map: addr=0x40c000, length=0x1000, pa=0x3bba1000
[0]uvm_alloc: map: addr=0x40d000, length=0x1000, pa=0x3bba0000
[0]uvm_alloc: map: addr=0x40e000, length=0x1000, pa=0x3bb9f000
[0]uvm_alloc: map: addr=0x40f000, length=0x1000, pa=0x3bb9e000
[0]uvm_alloc: map: addr=0x410000, length=0x1000, pa=0x3bb9d000
[0]uvm_alloc: map: addr=0x411000, length=0x1000, pa=0x3bb9c000
[0]uvm_alloc: map: addr=0x412000, length=0x1000, pa=0x3bb9b000
[0]uvm_alloc: map: addr=0x413000, length=0x1000, pa=0x3bb9a000
[0]uvm_alloc: map: addr=0x414000, length=0x1000, pa=0x3bb99000
[0]uvm_alloc: map: addr=0x415000, length=0x1000, pa=0x3bb98000
[0]uvm_alloc: map: addr=0x416000, length=0x1000, pa=0x3bb97000
[0]uvm_alloc: map: addr=0x417000, length=0x1000, pa=0x3bb96000
[0]uvm_alloc: map: addr=0x418000, length=0x1000, pa=0x3bb95000
[0]uvm_alloc: map: addr=0x419000, length=0x1000, pa=0x3bb94000
[0]uvm_alloc: map: addr=0x41a000, length=0x1000, pa=0x3bb93000
[0]uvm_alloc: map: addr=0x41b000, length=0x1000, pa=0x3bb92000
[0]uvm_alloc: map: addr=0x41c000, length=0x1000, pa=0x3bb91000
[0]uvm_alloc: map: addr=0x41d000, length=0x1000, pa=0x3bb90000
[0]uvm_alloc: map: addr=0x41e000, length=0x1000, pa=0x3bb8f000
[0]uvm_alloc: map: addr=0x41f000, length=0x1000, pa=0x3bb8e000
[0]uvm_alloc: map: addr=0x420000, length=0x1000, pa=0x3bb8d000
[0]uvm_alloc: map: addr=0x421000, length=0x1000, pa=0x3bb8c000
[0]uvm_alloc: map: addr=0x422000, length=0x1000, pa=0x3bb8b000
[0]uvm_alloc: map: addr=0x423000, length=0x1000, pa=0x3bb8a000
[0]uvm_alloc: map: addr=0x424000, length=0x1000, pa=0x3bb89000
[0]uvm_alloc: map: addr=0x425000, length=0x1000, pa=0x3bb88000
[0]uvm_alloc: map: addr=0x426000, length=0x1000, pa=0x3bb87000
[0]uvm_alloc: map: addr=0x427000, length=0x1000, pa=0x3bb86000
[0]uvm_alloc: map: addr=0x428000, length=0x1000, pa=0x3bb85000
[0]uvm_alloc: map: addr=0x429000, length=0x1000, pa=0x3bb84000
[0]uvm_alloc: map: addr=0x42a000, length=0x1000, pa=0x3bb83000
[0]uvm_alloc: map: addr=0x42b000, length=0x1000, pa=0x3bb82000
[0]uvm_alloc: map: addr=0x42c000, length=0x1000, pa=0x3bb81000
[0]uvm_alloc: map: addr=0x42d000, length=0x1000, pa=0x3bb80000
[0]uvm_alloc: map: addr=0x42e000, length=0x1000, pa=0x3bb7f000
[0]uvm_alloc: map: addr=0x42f000, length=0x1000, pa=0x3bb7e000
[0]execve: vm_free
[0]vm_free: free pte =0xffff00003bbfb000
[0]vm_free: free pte =0xffff00003bbc6000
[0]vm_free: free pte =0xffff00003bbc5000
[0]vm_free: free pte =0xffff00003bbc4000
[0]vm_free: free pte =0xffff00003bbc3000
[0]vm_free: free pte =0xffff00003bbc2000
[0]vm_free: free pte =0xffff00003bbc1000
[0]vm_free: free pte =0xffff00003bbc0000
[0]vm_free: free pte =0xffff00003bbbf000
[0]vm_free: free pgt3=0xffff00003bbc7000
[0]vm_free: free pgt2=0xffff00003bbfe000
[0]vm_free: free pgt1=0xffff00003bbfa000
[0]vm_free: free pte =0xffff00003bbbe000
[0]vm_free: free pte =0xffff00003bbba000
[0]vm_free: free pte =0xffff00003bbb9000
[0]vm_free: free pte =0xffff00003bbb8000
[0]vm_free: free pte =0xffff00003bbb7000
[0]vm_free: free pte =0xffff00003bbb6000
[0]vm_free: free pte =0xffff00003bbb5000
[0]vm_free: free pte =0xffff00003bbb4000
[0]vm_free: free pte =0xffff00003bbb3000
[0]vm_free: free pte =0xffff00003bbb2000
[0]vm_free: free pgt3=0xffff00003bbbb000
[0]vm_free: free pgt2=0xffff00003bbbc000
[0]vm_free: free pgt1=0xffff00003bbbd000
[0]vm_free: free dir =0xffff00003bbfc000
[0]vm_stat: va: 0x400000, pa: 0xffff00003bbb0000, pte: 0x3bbb0647, PTE_ADDR(pte): 0x3bbb0000
[0]vm_stat: va: 0x401000, pa: 0xffff00003bbac000, pte: 0x3bbac647, PTE_ADDR(pte): 0x3bbac000
[0]vm_stat: va: 0x402000, pa: 0xffff00003bbab000, pte: 0x3bbab647, PTE_ADDR(pte): 0x3bbab000
[0]vm_stat: va: 0x403000, pa: 0xffff00003bbaa000, pte: 0x3bbaa647, PTE_ADDR(pte): 0x3bbaa000
[0]vm_stat: va: 0x404000, pa: 0xffff00003bba9000, pte: 0x3bba9647, PTE_ADDR(pte): 0x3bba9000
[0]vm_stat: va: 0x405000, pa: 0xffff00003bba8000, pte: 0x3bba8647, PTE_ADDR(pte): 0x3bba8000
[0]vm_stat: va: 0x406000, pa: 0xffff00003bba7000, pte: 0x3bba7647, PTE_ADDR(pte): 0x3bba7000
[0]vm_stat: va: 0x407000, pa: 0xffff00003bba6000, pte: 0x3bba6647, PTE_ADDR(pte): 0x3bba6000
[0]vm_stat: va: 0x408000, pa: 0xffff00003bba5000, pte: 0x3bba5647, PTE_ADDR(pte): 0x3bba5000
[0]vm_stat: va: 0x409000, pa: 0xffff00003bba4000, pte: 0x3bba4647, PTE_ADDR(pte): 0x3bba4000
[0]vm_stat: va: 0x40a000, pa: 0xffff00003bba3000, pte: 0x3bba3647, PTE_ADDR(pte): 0x3bba3000
[0]vm_stat: va: 0x40b000, pa: 0xffff00003bba2000, pte: 0x3bba2647, PTE_ADDR(pte): 0x3bba2000
[0]vm_stat: va: 0x40c000, pa: 0xffff00003bba1000, pte: 0x3bba1647, PTE_ADDR(pte): 0x3bba1000
[0]vm_stat: va: 0x40d000, pa: 0xffff00003bba0000, pte: 0x3bba0647, PTE_ADDR(pte): 0x3bba0000
[0]vm_stat: va: 0x40e000, pa: 0xffff00003bb9f000, pte: 0x3bb9f647, PTE_ADDR(pte): 0x3bb9f000
[0]vm_stat: va: 0x40f000, pa: 0xffff00003bb9e000, pte: 0x3bb9e647, PTE_ADDR(pte): 0x3bb9e000
[0]vm_stat: va: 0x410000, pa: 0xffff00003bb9d000, pte: 0x3bb9d647, PTE_ADDR(pte): 0x3bb9d000
[0]vm_stat: va: 0x411000, pa: 0xffff00003bb9c000, pte: 0x3bb9c647, PTE_ADDR(pte): 0x3bb9c000
[0]vm_stat: va: 0x412000, pa: 0xffff00003bb9b000, pte: 0x3bb9b647, PTE_ADDR(pte): 0x3bb9b000
[0]vm_stat: va: 0x413000, pa: 0xffff00003bb9a000, pte: 0x3bb9a647, PTE_ADDR(pte): 0x3bb9a000
[0]vm_stat: va: 0x414000, pa: 0xffff00003bb99000, pte: 0x3bb99647, PTE_ADDR(pte): 0x3bb99000
[0]vm_stat: va: 0x415000, pa: 0xffff00003bb98000, pte: 0x3bb98647, PTE_ADDR(pte): 0x3bb98000
[0]vm_stat: va: 0x416000, pa: 0xffff00003bb97000, pte: 0x3bb97647, PTE_ADDR(pte): 0x3bb97000
[0]vm_stat: va: 0x417000, pa: 0xffff00003bb96000, pte: 0x3bb96647, PTE_ADDR(pte): 0x3bb96000
[0]vm_stat: va: 0x418000, pa: 0xffff00003bb95000, pte: 0x3bb95647, PTE_ADDR(pte): 0x3bb95000
[0]vm_stat: va: 0x419000, pa: 0xffff00003bb94000, pte: 0x3bb94647, PTE_ADDR(pte): 0x3bb94000
[0]vm_stat: va: 0x41a000, pa: 0xffff00003bb93000, pte: 0x3bb93647, PTE_ADDR(pte): 0x3bb93000
[0]vm_stat: va: 0x41b000, pa: 0xffff00003bb92000, pte: 0x3bb92647, PTE_ADDR(pte): 0x3bb92000
[0]vm_stat: va: 0x41c000, pa: 0xffff00003bb91000, pte: 0x3bb91647, PTE_ADDR(pte): 0x3bb91000
[0]vm_stat: va: 0x41d000, pa: 0xffff00003bb90000, pte: 0x3bb90647, PTE_ADDR(pte): 0x3bb90000
[0]vm_stat: va: 0x41e000, pa: 0xffff00003bb8f000, pte: 0x3bb8f647, PTE_ADDR(pte): 0x3bb8f000
[0]vm_stat: va: 0x41f000, pa: 0xffff00003bb8e000, pte: 0x3bb8e647, PTE_ADDR(pte): 0x3bb8e000
[0]vm_stat: va: 0x420000, pa: 0xffff00003bb8d000, pte: 0x3bb8d647, PTE_ADDR(pte): 0x3bb8d000
[0]vm_stat: va: 0x421000, pa: 0xffff00003bb8c000, pte: 0x3bb8c647, PTE_ADDR(pte): 0x3bb8c000
[0]vm_stat: va: 0x422000, pa: 0xffff00003bb8b000, pte: 0x3bb8b647, PTE_ADDR(pte): 0x3bb8b000
[0]vm_stat: va: 0x423000, pa: 0xffff00003bb8a000, pte: 0x3bb8a647, PTE_ADDR(pte): 0x3bb8a000
[0]vm_stat: va: 0x424000, pa: 0xffff00003bb89000, pte: 0x3bb89647, PTE_ADDR(pte): 0x3bb89000
[0]vm_stat: va: 0x425000, pa: 0xffff00003bb88000, pte: 0x3bb88647, PTE_ADDR(pte): 0x3bb88000
[0]vm_stat: va: 0x426000, pa: 0xffff00003bb87000, pte: 0x3bb87647, PTE_ADDR(pte): 0x3bb87000
[0]vm_stat: va: 0x427000, pa: 0xffff00003bb86000, pte: 0x3bb86647, PTE_ADDR(pte): 0x3bb86000
[0]vm_stat: va: 0x428000, pa: 0xffff00003bb85000, pte: 0x3bb85647, PTE_ADDR(pte): 0x3bb85000
[0]vm_stat: va: 0x429000, pa: 0xffff00003bb84000, pte: 0x3bb84647, PTE_ADDR(pte): 0x3bb84000
[0]vm_stat: va: 0x42a000, pa: 0xffff00003bb83000, pte: 0x3bb83647, PTE_ADDR(pte): 0x3bb83000
[0]vm_stat: va: 0x42b000, pa: 0xffff00003bb82000, pte: 0x3bb82647, PTE_ADDR(pte): 0x3bb82000
[0]vm_stat: va: 0x42c000, pa: 0xffff00003bb81000, pte: 0x3bb81647, PTE_ADDR(pte): 0x3bb81000
[0]vm_stat: va: 0x42d000, pa: 0xffff00003bb80000, pte: 0x3bb80647, PTE_ADDR(pte): 0x3bb80000
[0]vm_stat: va: 0x42e000, pa: 0xffff00003bb7f000, pte: 0x3bb7f647, PTE_ADDR(pte): 0x3bb7f000
[0]vm_stat: va: 0x42f000, pa: 0xffff00003bb7e000, pte: 0x3bb7e647, PTE_ADDR(pte): 0x3bb7e000
[0]vm_stat: va: 0xffffffff6000, pa: 0xffff00003bb79000, pte: 0x3bb79647, PTE_ADDR(pte): 0x3bb79000
[0]vm_stat: va: [0x400000 ~ 0x430000)
[0]vm_stat: va: 0xffffffff7000, pa: 0xffff00003bb78000, pte: 0x3bb78647, PTE_ADDR(pte): 0x3bb78000
[0]vm_stat: va: 0xffffffff8000, pa: 0xffff00003bb77000, pte: 0x3bb77647, PTE_ADDR(pte): 0x3bb77000
[0]vm_stat: va: 0xffffffff9000, pa: 0xffff00003bb76000, pte: 0x3bb76647, PTE_ADDR(pte): 0x3bb76000
[0]vm_stat: va: 0xffffffffa000, pa: 0xffff00003bb75000, pte: 0x3bb75647, PTE_ADDR(pte): 0x3bb75000
[0]vm_stat: va: 0xffffffffb000, pa: 0xffff00003bb74000, pte: 0x3bb74647, PTE_ADDR(pte): 0x3bb74000
[0]vm_stat: va: 0xffffffffc000, pa: 0xffff00003bb73000, pte: 0x3bb73647, PTE_ADDR(pte): 0x3bb73000
[0]vm_stat: va: 0xffffffffd000, pa: 0xffff00003bb72000, pte: 0x3bb72647, PTE_ADDR(pte): 0x3bb72000
[0]vm_stat: va: 0xffffffffe000, pa: 0xffff00003bb71000, pte: 0x3bb71647, PTE_ADDR(pte): 0x3bb71000
[0]vm_stat: va: 0xfffffffff000, pa: 0xffff00003bb7a000, pte: 0x3bb7a647, PTE_ADDR(pte): 0x3bb7a000
[0]vm_stat: va: [0xffffffff6000 ~ 0x1000000000000)
[1]uvm_alloc: map: addr=0x430000, length=0x1000, pa=0x3bbfc000
[1]uvm_alloc: map: addr=0x431000, length=0x1000, pa=0x3bbbd000
[1]map_anon_page: map: addr=0x600000000000, length=0x1000, pa=0x3bbbb000
# /bin/ls
[0]uvm_copy: map: addr=0x400000, length=0x1000, pa=0x3bbb7000
[0]uvm_copy: map: addr=0x401000, length=0x1000, pa=0x3bbbe000
[0]uvm_copy: map: addr=0x402000, length=0x1000, pa=0x3bbfa000
[0]uvm_copy: map: addr=0x403000, length=0x1000, pa=0x3bbfe000
[0]uvm_copy: map: addr=0x404000, length=0x1000, pa=0x3bbc7000
[0]uvm_copy: map: addr=0x405000, length=0x1000, pa=0x3bbbf000
[0]uvm_copy: map: addr=0x406000, length=0x1000, pa=0x3bbc0000
[0]uvm_copy: map: addr=0x407000, length=0x1000, pa=0x3bbc1000
[0]uvm_copy: map: addr=0x408000, length=0x1000, pa=0x3bbc2000
[0]uvm_copy: map: addr=0x409000, length=0x1000, pa=0x3bbc3000
[0]uvm_copy: map: addr=0x40a000, length=0x1000, pa=0x3bbc4000
[0]uvm_copy: map: addr=0x40b000, length=0x1000, pa=0x3bbc5000
[0]uvm_copy: map: addr=0x40c000, length=0x1000, pa=0x3bbc6000
[0]uvm_copy: map: addr=0x40d000, length=0x1000, pa=0x3bbfb000
[0]uvm_copy: map: addr=0x40e000, length=0x1000, pa=0x3bb70000
[0]uvm_copy: map: addr=0x40f000, length=0x1000, pa=0x3bb6f000
[0]uvm_copy: map: addr=0x410000, length=0x1000, pa=0x3bb6e000
[0]uvm_copy: map: addr=0x411000, length=0x1000, pa=0x3bb6d000
[0]uvm_copy: map: addr=0x412000, length=0x1000, pa=0x3bb6c000
[0]uvm_copy: map: addr=0x413000, length=0x1000, pa=0x3bb6b000
[0]uvm_copy: map: addr=0x414000, length=0x1000, pa=0x3bb6a000
[0]uvm_copy: map: addr=0x415000, length=0x1000, pa=0x3bb69000
[0]uvm_copy: map: addr=0x416000, length=0x1000, pa=0x3bb68000
[0]uvm_copy: map: addr=0x417000, length=0x1000, pa=0x3bb67000
[0]uvm_copy: map: addr=0x418000, length=0x1000, pa=0x3bb66000
[0]uvm_copy: map: addr=0x419000, length=0x1000, pa=0x3bb65000
[0]uvm_copy: map: addr=0x41a000, length=0x1000, pa=0x3bb64000
[0]uvm_copy: map: addr=0x41b000, length=0x1000, pa=0x3bb63000
[0]uvm_copy: map: addr=0x41c000, length=0x1000, pa=0x3bb62000
[0]uvm_copy: map: addr=0x41d000, length=0x1000, pa=0x3bb61000
[0]uvm_copy: map: addr=0x41e000, length=0x1000, pa=0x3bb60000
[0]uvm_copy: map: addr=0x41f000, length=0x1000, pa=0x3bb5f000
[0]uvm_copy: map: addr=0x420000, length=0x1000, pa=0x3bb5e000
[0]uvm_copy: map: addr=0x421000, length=0x1000, pa=0x3bb5d000
[0]uvm_copy: map: addr=0x422000, length=0x1000, pa=0x3bb5c000
[0]uvm_copy: map: addr=0x423000, length=0x1000, pa=0x3bb5b000
[0]uvm_copy: map: addr=0x424000, length=0x1000, pa=0x3bb5a000
[0]uvm_copy: map: addr=0x425000, length=0x1000, pa=0x3bb59000
[0]uvm_copy: map: addr=0x426000, length=0x1000, pa=0x3bb58000
[0]uvm_copy: map: addr=0x427000, length=0x1000, pa=0x3bb57000
[0]uvm_copy: map: addr=0x428000, length=0x1000, pa=0x3bb56000
[0]uvm_copy: map: addr=0x429000, length=0x1000, pa=0x3bb55000
[0]uvm_copy: map: addr=0x42a000, length=0x1000, pa=0x3bb54000
[0]uvm_copy: map: addr=0x42b000, length=0x1000, pa=0x3bb53000
[0]uvm_copy: map: addr=0x42c000, length=0x1000, pa=0x3bb52000
[0]uvm_copy: map: addr=0x42d000, length=0x1000, pa=0x3bb51000
[0]uvm_copy: map: addr=0x42e000, length=0x1000, pa=0x3bb50000
[0]uvm_copy: map: addr=0x42f000, length=0x1000, pa=0x3bb4f000
[0]uvm_copy: map: addr=0x430000, length=0x1000, pa=0x3bb4e000
[0]uvm_copy: map: addr=0x431000, length=0x1000, pa=0x3bb4d000
[0]uvm_copy: map: addr=0x600000000000, length=0x1000, pa=0x3bb4c000
[0]uvm_copy: map: addr=0xffffffff6000, length=0x1000, pa=0x3bb48000
[0]uvm_copy: map: addr=0xffffffff7000, length=0x1000, pa=0x3bb44000
[0]uvm_copy: map: addr=0xffffffff8000, length=0x1000, pa=0x3bb43000
[0]uvm_copy: map: addr=0xffffffff9000, length=0x1000, pa=0x3bb42000
[0]uvm_copy: map: addr=0xffffffffa000, length=0x1000, pa=0x3bb41000
[0]uvm_copy: map: addr=0xffffffffb000, length=0x1000, pa=0x3bb40000
[0]uvm_copy: map: addr=0xffffffffc000, length=0x1000, pa=0x3bb3f000
[0]uvm_copy: map: addr=0xffffffffd000, length=0x1000, pa=0x3bb3e000
[0]uvm_copy: map: addr=0xffffffffe000, length=0x1000, pa=0x3bb3d000
[0]uvm_copy: map: addr=0xfffffffff000, length=0x1000, pa=0x3bb3c000
[2]uvm_alloc: map: addr=0x400000, length=0x1000, pa=0x3bb3a000
[2]uvm_alloc: map: addr=0x401000, length=0x1000, pa=0x3bb36000
[2]uvm_alloc: map: addr=0x402000, length=0x1000, pa=0x3bb35000
[2]uvm_alloc: map: addr=0x403000, length=0x1000, pa=0x3bb34000
[2]uvm_alloc: map: addr=0x404000, length=0x1000, pa=0x3bb33000
[2]uvm_alloc: map: addr=0x405000, length=0x1000, pa=0x3bb32000
[2]uvm_alloc: map: addr=0x406000, length=0x1000, pa=0x3bb31000
[2]uvm_alloc: map: addr=0x407000, length=0x1000, pa=0x3bb30000
[2]uvm_alloc: map: addr=0x408000, length=0x1000, pa=0x3bb2f000
[2]uvm_alloc: map: addr=0x409000, length=0x1000, pa=0x3bb2e000
[2]uvm_alloc: map: addr=0x40a000, length=0x1000, pa=0x3bb2d000
[2]execve: vm_free
[2]vm_free: free pte =0xffff00003bbb7000
[2]vm_free: free pte =0xffff00003bbbe000
[2]vm_free: free pte =0xffff00003bbfa000
[2]vm_free: free pte =0xffff00003bbfe000
[2]vm_free: free pte =0xffff00003bbc7000
[2]vm_free: free pte =0xffff00003bbbf000
[2]vm_free: free pte =0xffff00003bbc0000
[2]vm_free: free pte =0xffff00003bbc1000
[2]vm_free: free pte =0xffff00003bbc2000
[2]vm_free: free pte =0xffff00003bbc3000
[2]vm_free: free pte =0xffff00003bbc4000
[2]vm_free: free pte =0xffff00003bbc5000
[2]vm_free: free pte =0xffff00003bbc6000
[2]vm_free: free pte =0xffff00003bbfb000
[2]vm_free: free pte =0xffff00003bb70000
[2]vm_free: free pte =0xffff00003bb6f000
[2]vm_free: free pte =0xffff00003bb6e000
[2]vm_free: free pte =0xffff00003bb6d000
[2]vm_free: free pte =0xffff00003bb6c000
[2]vm_free: free pte =0xffff00003bb6b000
[2]vm_free: free pte =0xffff00003bb6a000
[2]vm_free: free pte =0xffff00003bb69000
[2]vm_free: free pte =0xffff00003bb68000
[2]vm_free: free pte =0xffff00003bb67000
[2]vm_free: free pte =0xffff00003bb66000
[2]vm_free: free pte =0xffff00003bb65000
[2]vm_free: free pte =0xffff00003bb64000
[2]vm_free: free pte =0xffff00003bb63000
[2]vm_free: free pte =0xffff00003bb62000
[2]vm_free: free pte =0xffff00003bb61000
[2]vm_free: free pte =0xffff00003bb60000
[2]vm_free: free pte =0xffff00003bb5f000
[2]vm_free: free pte =0xffff00003bb5e000
[2]vm_free: free pte =0xffff00003bb5d000
[2]vm_free: free pte =0xffff00003bb5c000
[2]vm_free: free pte =0xffff00003bb5b000
[2]vm_free: free pte =0xffff00003bb5a000
[2]vm_free: free pte =0xffff00003bb59000
[2]vm_free: free pte =0xffff00003bb58000
[2]vm_free: free pte =0xffff00003bb57000
[2]vm_free: free pte =0xffff00003bb56000
[2]vm_free: free pte =0xffff00003bb55000
[2]vm_free: free pte =0xffff00003bb54000
[2]vm_free: free pte =0xffff00003bb53000
[2]vm_free: free pte =0xffff00003bb52000
[2]vm_free: free pte =0xffff00003bb51000
[2]vm_free: free pte =0xffff00003bb50000
[2]vm_free: free pte =0xffff00003bb4f000
[2]vm_free: free pte =0xffff00003bb4e000
[2]vm_free: free pte =0xffff00003bb4d000
[2]vm_free: free pgt3=0xffff00003bbba000
[2]vm_free: free pgt2=0xffff00003bbb9000
[2]vm_free: free pgt1=0xffff00003bbb8000
[2]vm_free: free pte =0xffff00003bb4c000
[2]vm_free: free pgt3=0xffff00003bb49000
[2]vm_free: free pgt2=0xffff00003bb4a000
[2]vm_free: free pgt1=0xffff00003bb4b000
[2]vm_free: free pte =0xffff00003bb48000
[2]vm_free: free pte =0xffff00003bb44000
[2]vm_free: free pte =0xffff00003bb43000
[2]vm_free: free pte =0xffff00003bb42000
[2]vm_free: free pte =0xffff00003bb41000
[2]vm_free: free pte =0xffff00003bb40000
[2]vm_free: free pte =0xffff00003bb3f000
[2]vm_free: free pte =0xffff00003bb3e000
[2]vm_free: free pte =0xffff00003bb3d000
[2]vm_free: free pte =0xffff00003bb3c000
[2]vm_free: free pgt3=0xffff00003bb45000
[2]vm_free: free pgt2=0xffff00003bb46000
[2]vm_free: free pgt1=0xffff00003bb47000
[2]vm_free: free dir =0xffff00003bbb6000
[2]vm_stat: va: 0x400000, pa: 0xffff00003bb3a000, pte: 0x3bb3a647, PTE_ADDR(pte): 0x3bb3a000
[2]vm_stat: va: 0x401000, pa: 0xffff00003bb36000, pte: 0x3bb36647, PTE_ADDR(pte): 0x3bb36000
[2]vm_stat: va: 0x402000, pa: 0xffff00003bb35000, pte: 0x3bb35647, PTE_ADDR(pte): 0x3bb35000
[2]vm_stat: va: 0x403000, pa: 0xffff00003bb34000, pte: 0x3bb34647, PTE_ADDR(pte): 0x3bb34000
[2]vm_stat: va: 0x404000, pa: 0xffff00003bb33000, pte: 0x3bb33647, PTE_ADDR(pte): 0x3bb33000
[2]vm_stat: va: 0x405000, pa: 0xffff00003bb32000, pte: 0x3bb32647, PTE_ADDR(pte): 0x3bb32000
[2]vm_stat: va: 0x406000, pa: 0xffff00003bb31000, pte: 0x3bb31647, PTE_ADDR(pte): 0x3bb31000
[2]vm_stat: va: 0x407000, pa: 0xffff00003bb30000, pte: 0x3bb30647, PTE_ADDR(pte): 0x3bb30000
[2]vm_stat: va: 0x408000, pa: 0xffff00003bb2f000, pte: 0x3bb2f647, PTE_ADDR(pte): 0x3bb2f000
[2]vm_stat: va: 0x409000, pa: 0xffff00003bb2e000, pte: 0x3bb2e647, PTE_ADDR(pte): 0x3bb2e000
[2]vm_stat: va: 0x40a000, pa: 0xffff00003bb2d000, pte: 0x3bb2d647, PTE_ADDR(pte): 0x3bb2d000
[2]vm_stat: va: 0xffffffff6000, pa: 0xffff00003bb28000, pte: 0x3bb28647, PTE_ADDR(pte): 0x3bb28000
[2]vm_stat: va: [0x400000 ~ 0x40b000)
[2]vm_stat: va: 0xffffffff7000, pa: 0xffff00003bb27000, pte: 0x3bb27647, PTE_ADDR(pte): 0x3bb27000
[2]vm_stat: va: 0xffffffff8000, pa: 0xffff00003bb26000, pte: 0x3bb26647, PTE_ADDR(pte): 0x3bb26000
[2]vm_stat: va: 0xffffffff9000, pa: 0xffff00003bb25000, pte: 0x3bb25647, PTE_ADDR(pte): 0x3bb25000
[2]vm_stat: va: 0xffffffffa000, pa: 0xffff00003bb24000, pte: 0x3bb24647, PTE_ADDR(pte): 0x3bb24000
[2]vm_stat: va: 0xffffffffb000, pa: 0xffff00003bb23000, pte: 0x3bb23647, PTE_ADDR(pte): 0x3bb23000
[2]vm_stat: va: 0xffffffffc000, pa: 0xffff00003bb22000, pte: 0x3bb22647, PTE_ADDR(pte): 0x3bb22000
[2]vm_stat: va: 0xffffffffd000, pa: 0xffff00003bb21000, pte: 0x3bb21647, PTE_ADDR(pte): 0x3bb21000
[2]vm_stat: va: 0xffffffffe000, pa: 0xffff00003bb20000, pte: 0x3bb20647, PTE_ADDR(pte): 0x3bb20000
[2]vm_stat: va: 0xfffffffff000, pa: 0xffff00003bb29000, pte: 0x3bb29647, PTE_ADDR(pte): 0x3bb29000
[2]vm_stat: va: [0xffffffff6000 ~ 0x1000000000000)
drwxrwxr-x    1 root wheel  1024  7  9 09:03 .
drwxrwxr-x    1 root wheel  1024  7  9 09:03 ..
drwxrwxr-x    2 root wheel  1024  7  9 09:03 bin
drwxrwxr-x    3 root wheel   384  7  9 09:03 dev
drwxrwxr-x    8 root wheel   256  7  9 09:03 etc
drwxrwxrwx    9 root wheel   128  7  9 09:03 lib
drwxrwxr-x   10 root wheel   192  7  9 09:03 home
drwxrwxr-x   12 root wheel   256  7  9 09:03 usr
-rwxr-xr-x   30 root wheel    94  7  9 09:03 test.txt
[0]wait4: vm_free
[0]vm_free: free pte =0xffff00003bb3a000
[0]vm_free: free pte =0xffff00003bb36000
[0]vm_free: free pte =0xffff00003bb35000
[0]vm_free: free pte =0xffff00003bb34000
[0]vm_free: free pte =0xffff00003bb33000
[0]vm_free: free pte =0xffff00003bb32000
[0]vm_free: free pte =0xffff00003bb31000
[0]vm_free: free pte =0xffff00003bb30000
[0]vm_free: free pte =0xffff00003bb2f000
[0]vm_free: free pte =0xffff00003bb2e000
[0]vm_free: free pte =0xffff00003bb2d000
[0]vm_free: free pgt3=0xffff00003bb37000
[0]vm_free: free pgt2=0xffff00003bb38000
[0]vm_free: free pgt1=0xffff00003bb39000
[0]vm_free: free pte =0xffff00003bb28000
[0]vm_free: free pte =0xffff00003bb27000
[0]vm_free: free pte =0xffff00003bb26000
[0]vm_free: free pte =0xffff00003bb25000
[0]vm_free: free pte =0xffff00003bb24000
[0]vm_free: free pte =0xffff00003bb23000
[0]vm_free: free pte =0xffff00003bb22000
[0]vm_free: free pte =0xffff00003bb21000
[0]vm_free: free pte =0xffff00003bb20000
[0]vm_free: free pte =0xffff00003bb29000
[0]vm_free: free pgt3=0xffff00003bb2a000
[0]vm_free: free pgt2=0xffff00003bb2b000
[0]vm_free: free pgt1=0xffff00003bb2c000
[0]vm_free: free dir =0xffff00003bb3b000
[0]map_anon_page: map: addr=0x600000001000, length=0x1000, pa=0x3bbb5000
# [0]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[0]uvm_unmap: free va=0x600000001000, pa=0xffff00003bbb5000, *pte=0x3bbb5000
mmaptest2
[0]uvm_copy: map: addr=0x400000, length=0x1000, pa=0x3bb2c000
[0]uvm_copy: map: addr=0x401000, length=0x1000, pa=0x3bb20000
[0]uvm_copy: map: addr=0x402000, length=0x1000, pa=0x3bb21000
[0]uvm_copy: map: addr=0x403000, length=0x1000, pa=0x3bb22000
[0]uvm_copy: map: addr=0x404000, length=0x1000, pa=0x3bb23000
[0]uvm_copy: map: addr=0x405000, length=0x1000, pa=0x3bb24000
[0]uvm_copy: map: addr=0x406000, length=0x1000, pa=0x3bb25000
[0]uvm_copy: map: addr=0x407000, length=0x1000, pa=0x3bb26000
[0]uvm_copy: map: addr=0x408000, length=0x1000, pa=0x3bb27000
[0]uvm_copy: map: addr=0x409000, length=0x1000, pa=0x3bb28000
[0]uvm_copy: map: addr=0x40a000, length=0x1000, pa=0x3bb39000
[0]uvm_copy: map: addr=0x40b000, length=0x1000, pa=0x3bb38000
[0]uvm_copy: map: addr=0x40c000, length=0x1000, pa=0x3bb37000
[0]uvm_copy: map: addr=0x40d000, length=0x1000, pa=0x3bb2d000
[0]uvm_copy: map: addr=0x40e000, length=0x1000, pa=0x3bb2e000
[0]uvm_copy: map: addr=0x40f000, length=0x1000, pa=0x3bb2f000
[0]uvm_copy: map: addr=0x410000, length=0x1000, pa=0x3bb30000
[0]uvm_copy: map: addr=0x411000, length=0x1000, pa=0x3bb31000
[0]uvm_copy: map: addr=0x412000, length=0x1000, pa=0x3bb32000
[0]uvm_copy: map: addr=0x413000, length=0x1000, pa=0x3bb33000
[0]uvm_copy: map: addr=0x414000, length=0x1000, pa=0x3bb34000
[0]uvm_copy: map: addr=0x415000, length=0x1000, pa=0x3bb35000
[0]uvm_copy: map: addr=0x416000, length=0x1000, pa=0x3bb36000
[0]uvm_copy: map: addr=0x417000, length=0x1000, pa=0x3bb3a000
[0]uvm_copy: map: addr=0x418000, length=0x1000, pa=0x3bbb6000
[0]uvm_copy: map: addr=0x419000, length=0x1000, pa=0x3bb47000
[0]uvm_copy: map: addr=0x41a000, length=0x1000, pa=0x3bb46000
[0]uvm_copy: map: addr=0x41b000, length=0x1000, pa=0x3bb45000
[0]uvm_copy: map: addr=0x41c000, length=0x1000, pa=0x3bb3c000
[0]uvm_copy: map: addr=0x41d000, length=0x1000, pa=0x3bb3d000
[0]uvm_copy: map: addr=0x41e000, length=0x1000, pa=0x3bb3e000
[0]uvm_copy: map: addr=0x41f000, length=0x1000, pa=0x3bb3f000
[0]uvm_copy: map: addr=0x420000, length=0x1000, pa=0x3bb40000
[0]uvm_copy: map: addr=0x421000, length=0x1000, pa=0x3bb41000
[0]uvm_copy: map: addr=0x422000, length=0x1000, pa=0x3bb42000
[0]uvm_copy: map: addr=0x423000, length=0x1000, pa=0x3bb43000
[0]uvm_copy: map: addr=0x424000, length=0x1000, pa=0x3bb44000
[0]uvm_copy: map: addr=0x425000, length=0x1000, pa=0x3bb48000
[0]uvm_copy: map: addr=0x426000, length=0x1000, pa=0x3bb4b000
[0]uvm_copy: map: addr=0x427000, length=0x1000, pa=0x3bb4a000
[0]uvm_copy: map: addr=0x428000, length=0x1000, pa=0x3bb49000
[0]uvm_copy: map: addr=0x429000, length=0x1000, pa=0x3bb4c000
[0]uvm_copy: map: addr=0x42a000, length=0x1000, pa=0x3bbb8000
[0]uvm_copy: map: addr=0x42b000, length=0x1000, pa=0x3bbb9000
[0]uvm_copy: map: addr=0x42c000, length=0x1000, pa=0x3bbba000
[0]uvm_copy: map: addr=0x42d000, length=0x1000, pa=0x3bb4d000
[0]uvm_copy: map: addr=0x42e000, length=0x1000, pa=0x3bb4e000
[0]uvm_copy: map: addr=0x42f000, length=0x1000, pa=0x3bb4f000
[0]uvm_copy: map: addr=0x430000, length=0x1000, pa=0x3bb50000
[0]uvm_copy: map: addr=0x431000, length=0x1000, pa=0x3bb51000
[0]uvm_copy: map: addr=0x600000000000, length=0x1000, pa=0x3bb52000
[0]uvm_copy: map: addr=0xffffffff6000, length=0x1000, pa=0x3bb56000
[0]uvm_copy: map: addr=0xffffffff7000, length=0x1000, pa=0x3bb5a000
[0]uvm_copy: map: addr=0xffffffff8000, length=0x1000, pa=0x3bb5b000
[0]uvm_copy: map: addr=0xffffffff9000, length=0x1000, pa=0x3bb5c000
[0]uvm_copy: map: addr=0xffffffffa000, length=0x1000, pa=0x3bb5d000
[0]uvm_copy: map: addr=0xffffffffb000, length=0x1000, pa=0x3bb5e000
[0]uvm_copy: map: addr=0xffffffffc000, length=0x1000, pa=0x3bb5f000
[0]uvm_copy: map: addr=0xffffffffd000, length=0x1000, pa=0x3bb60000
[0]uvm_copy: map: addr=0xffffffffe000, length=0x1000, pa=0x3bb61000
[0]uvm_copy: map: addr=0xfffffffff000, length=0x1000, pa=0x3bb62000
[0]uvm_alloc: map: addr=0x400000, length=0x1000, pa=0x3bb64000
[0]uvm_alloc: map: addr=0x401000, length=0x1000, pa=0x3bb68000
[0]uvm_alloc: map: addr=0x402000, length=0x1000, pa=0x3bb69000
[0]uvm_alloc: map: addr=0x403000, length=0x1000, pa=0x3bb6a000
[0]uvm_alloc: map: addr=0x404000, length=0x1000, pa=0x3bb6b000
[0]uvm_alloc: map: addr=0x405000, length=0x1000, pa=0x3bb6c000
[0]uvm_alloc: map: addr=0x406000, length=0x1000, pa=0x3bb6d000
[0]uvm_alloc: map: addr=0x407000, length=0x1000, pa=0x3bb6e000
[0]uvm_alloc: map: addr=0x408000, length=0x1000, pa=0x3bb6f000
[0]uvm_alloc: map: addr=0x409000, length=0x1000, pa=0x3bb70000
[0]uvm_alloc: map: addr=0x40a000, length=0x1000, pa=0x3bbfb000
[0]uvm_alloc: map: addr=0x40b000, length=0x1000, pa=0x3bbc6000
[0]uvm_alloc: map: addr=0x40c000, length=0x1000, pa=0x3bbc5000
[0]uvm_alloc: map: addr=0x40d000, length=0x1000, pa=0x3bbc4000
[0]uvm_alloc: map: addr=0x40e000, length=0x1000, pa=0x3bbc3000
[0]uvm_alloc: map: addr=0x40f000, length=0x1000, pa=0x3bbc2000
[0]uvm_alloc: map: addr=0x410000, length=0x1000, pa=0x3bbc1000
[0]uvm_alloc: map: addr=0x411000, length=0x1000, pa=0x3bbc0000
[0]uvm_alloc: map: addr=0x412000, length=0x1000, pa=0x3bbbf000
[0]execve: vm_free
[0]vm_free: free pte =0xffff00003bb2c000
[0]vm_free: free pte =0xffff00003bb20000
[0]vm_free: free pte =0xffff00003bb21000
[0]vm_free: free pte =0xffff00003bb22000
[0]vm_free: free pte =0xffff00003bb23000
[0]vm_free: free pte =0xffff00003bb24000
[0]vm_free: free pte =0xffff00003bb25000
[0]vm_free: free pte =0xffff00003bb26000
[0]vm_free: free pte =0xffff00003bb27000
[0]vm_free: free pte =0xffff00003bb28000
[0]vm_free: free pte =0xffff00003bb39000
[0]vm_free: free pte =0xffff00003bb38000
[0]vm_free: free pte =0xffff00003bb37000
[0]vm_free: free pte =0xffff00003bb2d000
[0]vm_free: free pte =0xffff00003bb2e000
[0]vm_free: free pte =0xffff00003bb2f000
[0]vm_free: free pte =0xffff00003bb30000
[0]vm_free: free pte =0xffff00003bb31000
[0]vm_free: free pte =0xffff00003bb32000
[0]vm_free: free pte =0xffff00003bb33000
[0]vm_free: free pte =0xffff00003bb34000
[0]vm_free: free pte =0xffff00003bb35000
[0]vm_free: free pte =0xffff00003bb36000
[0]vm_free: free pte =0xffff00003bb3a000
[0]vm_free: free pte =0xffff00003bbb6000
[0]vm_free: free pte =0xffff00003bb47000
[0]vm_free: free pte =0xffff00003bb46000
[0]vm_free: free pte =0xffff00003bb45000
[0]vm_free: free pte =0xffff00003bb3c000
[0]vm_free: free pte =0xffff00003bb3d000
[0]vm_free: free pte =0xffff00003bb3e000
[0]vm_free: free pte =0xffff00003bb3f000
[0]vm_free: free pte =0xffff00003bb40000
[0]vm_free: free pte =0xffff00003bb41000
[0]vm_free: free pte =0xffff00003bb42000
[0]vm_free: free pte =0xffff00003bb43000
[0]vm_free: free pte =0xffff00003bb44000
[0]vm_free: free pte =0xffff00003bb48000
[0]vm_free: free pte =0xffff00003bb4b000
[0]vm_free: free pte =0xffff00003bb4a000
[0]vm_free: free pte =0xffff00003bb49000
[0]vm_free: free pte =0xffff00003bb4c000
[0]vm_free: free pte =0xffff00003bbb8000
[0]vm_free: free pte =0xffff00003bbb9000
[0]vm_free: free pte =0xffff00003bbba000
[0]vm_free: free pte =0xffff00003bb4d000
[0]vm_free: free pte =0xffff00003bb4e000
[0]vm_free: free pte =0xffff00003bb4f000
[0]vm_free: free pte =0xffff00003bb50000
[0]vm_free: free pte =0xffff00003bb51000
[0]vm_free: free pgt3=0xffff00003bb29000
[0]vm_free: free pgt2=0xffff00003bb2a000
[0]vm_free: free pgt1=0xffff00003bb2b000
[0]vm_free: free pte =0xffff00003bb52000
[0]vm_free: free pgt3=0xffff00003bb55000
[0]vm_free: free pgt2=0xffff00003bb54000
[0]vm_free: free pgt1=0xffff00003bb53000
[0]vm_free: free pte =0xffff00003bb56000
[0]vm_free: free pte =0xffff00003bb5a000
[0]vm_free: free pte =0xffff00003bb5b000
[0]vm_free: free pte =0xffff00003bb5c000
[0]vm_free: free pte =0xffff00003bb5d000
[0]vm_free: free pte =0xffff00003bb5e000
[0]vm_free: free pte =0xffff00003bb5f000
[0]vm_free: free pte =0xffff00003bb60000
[0]vm_free: free pte =0xffff00003bb61000
[0]vm_free: free pte =0xffff00003bb62000
[0]vm_free: free pgt3=0xffff00003bb59000
[0]vm_free: free pgt2=0xffff00003bb58000
[0]vm_free: free pgt1=0xffff00003bb57000
[0]vm_free: free dir =0xffff00003bb3b000
[0]vm_stat: va: 0x400000, pa: 0xffff00003bb64000, pte: 0x3bb64647, PTE_ADDR(pte): 0x3bb64000
[0]vm_stat: va: 0x401000, pa: 0xffff00003bb68000, pte: 0x3bb68647, PTE_ADDR(pte): 0x3bb68000
[0]vm_stat: va: 0x402000, pa: 0xffff00003bb69000, pte: 0x3bb69647, PTE_ADDR(pte): 0x3bb69000
[0]vm_stat: va: 0x403000, pa: 0xffff00003bb6a000, pte: 0x3bb6a647, PTE_ADDR(pte): 0x3bb6a000
[0]vm_stat: va: 0x404000, pa: 0xffff00003bb6b000, pte: 0x3bb6b647, PTE_ADDR(pte): 0x3bb6b000
[0]vm_stat: va: 0x405000, pa: 0xffff00003bb6c000, pte: 0x3bb6c647, PTE_ADDR(pte): 0x3bb6c000
[0]vm_stat: va: 0x406000, pa: 0xffff00003bb6d000, pte: 0x3bb6d647, PTE_ADDR(pte): 0x3bb6d000
[0]vm_stat: va: 0x407000, pa: 0xffff00003bb6e000, pte: 0x3bb6e647, PTE_ADDR(pte): 0x3bb6e000
[0]vm_stat: va: 0x408000, pa: 0xffff00003bb6f000, pte: 0x3bb6f647, PTE_ADDR(pte): 0x3bb6f000
[0]vm_stat: va: 0x409000, pa: 0xffff00003bb70000, pte: 0x3bb70647, PTE_ADDR(pte): 0x3bb70000
[0]vm_stat: va: 0x40a000, pa: 0xffff00003bbfb000, pte: 0x3bbfb647, PTE_ADDR(pte): 0x3bbfb000
[0]vm_stat: va: 0x40b000, pa: 0xffff00003bbc6000, pte: 0x3bbc6647, PTE_ADDR(pte): 0x3bbc6000
[0]vm_stat: va: 0x40c000, pa: 0xffff00003bbc5000, pte: 0x3bbc5647, PTE_ADDR(pte): 0x3bbc5000
[0]vm_stat: va: 0x40d000, pa: 0xffff00003bbc4000, pte: 0x3bbc4647, PTE_ADDR(pte): 0x3bbc4000
[0]vm_stat: va: 0x40e000, pa: 0xffff00003bbc3000, pte: 0x3bbc3647, PTE_ADDR(pte): 0x3bbc3000
[0]vm_stat: va: 0x40f000, pa: 0xffff00003bbc2000, pte: 0x3bbc2647, PTE_ADDR(pte): 0x3bbc2000
[0]vm_stat: va: 0x410000, pa: 0xffff00003bbc1000, pte: 0x3bbc1647, PTE_ADDR(pte): 0x3bbc1000
[0]vm_stat: va: 0x411000, pa: 0xffff00003bbc0000, pte: 0x3bbc0647, PTE_ADDR(pte): 0x3bbc0000
[0]vm_stat: va: 0x412000, pa: 0xffff00003bbbf000, pte: 0x3bbbf647, PTE_ADDR(pte): 0x3bbbf000
[0]vm_stat: va: 0xffffffff6000, pa: 0xffff00003bbb7000, pte: 0x3bbb7647, PTE_ADDR(pte): 0x3bbb7000
[0]vm_stat: va: [0x400000 ~ 0x413000)
[0]vm_stat: va: 0xffffffff7000, pa: 0xffff00003bb1f000, pte: 0x3bb1f647, PTE_ADDR(pte): 0x3bb1f000
[0]vm_stat: va: 0xffffffff8000, pa: 0xffff00003bb1e000, pte: 0x3bb1e647, PTE_ADDR(pte): 0x3bb1e000
[0]vm_stat: va: 0xffffffff9000, pa: 0xffff00003bb1d000, pte: 0x3bb1d647, PTE_ADDR(pte): 0x3bb1d000
[0]vm_stat: va: 0xffffffffa000, pa: 0xffff00003bb1c000, pte: 0x3bb1c647, PTE_ADDR(pte): 0x3bb1c000
[0]vm_stat: va: 0xffffffffb000, pa: 0xffff00003bb1b000, pte: 0x3bb1b647, PTE_ADDR(pte): 0x3bb1b000
[0]vm_stat: va: 0xffffffffc000, pa: 0xffff00003bb1a000, pte: 0x3bb1a647, PTE_ADDR(pte): 0x3bb1a000
[0]vm_stat: va: 0xffffffffd000, pa: 0xffff00003bb19000, pte: 0x3bb19647, PTE_ADDR(pte): 0x3bb19000
[0]vm_stat: va: 0xffffffffe000, pa: 0xffff00003bb18000, pte: 0x3bb18647, PTE_ADDR(pte): 0x3bb18000
[0]vm_stat: va: 0xfffffffff000, pa: 0xffff00003bbbe000, pte: 0x3bbbe647, PTE_ADDR(pte): 0x3bbbe000
[0]vm_stat: va: [0xffffffff6000 ~ 0x1000000000000)

[F-09] ファイルが背後にあるプライベートマッピング
[2]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb3b000
[2]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[2]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb3b000, *pte=0x3bb3b000
[F-09] ok

[F-10] ファイルが背後にある共有マッピングのテスト
[2]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb3b000
[2]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[2]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb3b000, *pte=0x3bb3b000
[F-10] ok

[F-11] file backed mapping pagecache coherency test
[2]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb3b000
[3]map_file_page: map: addr=0x600000002000, length=0x1000, pa=0x3bb62000
[3]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[3]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb3b000, *pte=0x3bb3b000
[3]delete_mmap_node: unmap: addr=0x600000002000, page=1, free=1
[3]uvm_unmap: free va=0x600000002000, pa=0xffff00003bb62000, *pte=0x3bb62000
[F-11] ok

[F-12] file backed private mapping with fork test
[1]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb62000
[1]uvm_copy: map: addr=0x400000, length=0x1000, pa=0x3bb60000
[1]uvm_copy: map: addr=0x401000, length=0x1000, pa=0x3bb5c000
[1]uvm_copy: map: addr=0x402000, length=0x1000, pa=0x3bb5b000
[1]uvm_copy: map: addr=0x403000, length=0x1000, pa=0x3bb5a000
[1]uvm_copy: map: addr=0x404000, length=0x1000, pa=0x3bb56000
[1]uvm_copy: map: addr=0x405000, length=0x1000, pa=0x3bb53000
[1]uvm_copy: map: addr=0x406000, length=0x1000, pa=0x3bb54000
[1]uvm_copy: map: addr=0x407000, length=0x1000, pa=0x3bb55000
[1]uvm_copy: map: addr=0x408000, length=0x1000, pa=0x3bb52000
[1]uvm_copy: map: addr=0x409000, length=0x1000, pa=0x3bb2b000
[1]uvm_copy: map: addr=0x40a000, length=0x1000, pa=0x3bb2a000
[1]uvm_copy: map: addr=0x40b000, length=0x1000, pa=0x3bb29000
[1]uvm_copy: map: addr=0x40c000, length=0x1000, pa=0x3bb51000
[1]uvm_copy: map: addr=0x40d000, length=0x1000, pa=0x3bb50000
[1]uvm_copy: map: addr=0x40e000, length=0x1000, pa=0x3bb4f000
[1]uvm_copy: map: addr=0x40f000, length=0x1000, pa=0x3bb4e000
[1]uvm_copy: map: addr=0x410000, length=0x1000, pa=0x3bb4d000
[1]uvm_copy: map: addr=0x411000, length=0x1000, pa=0x3bbba000
[1]uvm_copy: map: addr=0x412000, length=0x1000, pa=0x3bbb9000
[1]uvm_copy: map: addr=0x600000001000, length=0x1000, pa=0x3bbb8000
[1]uvm_copy: map: addr=0xffffffff6000, length=0x1000, pa=0x3bb4b000
[1]uvm_copy: map: addr=0xffffffff7000, length=0x1000, pa=0x3bb42000
[1]uvm_copy: map: addr=0xffffffff8000, length=0x1000, pa=0x3bb41000
[1]uvm_copy: map: addr=0xffffffff9000, length=0x1000, pa=0x3bb40000
[1]uvm_copy: map: addr=0xffffffffa000, length=0x1000, pa=0x3bb3f000
[1]uvm_copy: map: addr=0xffffffffb000, length=0x1000, pa=0x3bb3e000
[1]uvm_copy: map: addr=0xffffffffc000, length=0x1000, pa=0x3bb3d000
[1]uvm_copy: map: addr=0xffffffffd000, length=0x1000, pa=0x3bb3c000
[1]uvm_copy: map: addr=0xffffffffe000, length=0x1000, pa=0x3bb45000
[1]uvm_copy: map: addr=0xfffffffff000, length=0x1000, pa=0x3bb46000
[1]wait4: vm_free
[1]vm_free: free pte =0xffff00003bb60000
[1]vm_free: free pte =0xffff00003bb5c000
[1]vm_free: free pte =0xffff00003bb5b000
[1]vm_free: free pte =0xffff00003bb5a000
[1]vm_free: free pte =0xffff00003bb56000
[1]vm_free: free pte =0xffff00003bb53000
[1]vm_free: free pte =0xffff00003bb54000
[1]vm_free: free pte =0xffff00003bb55000
[1]vm_free: free pte =0xffff00003bb52000
[1]vm_free: free pte =0xffff00003bb2b000
[1]vm_free: free pte =0xffff00003bb2a000
[1]vm_free: free pte =0xffff00003bb29000
[1]vm_free: free pte =0xffff00003bb51000
[1]vm_free: free pte =0xffff00003bb50000
[1]vm_free: free pte =0xffff00003bb4f000
[1]vm_free: free pte =0xffff00003bb4e000
[1]vm_free: free pte =0xffff00003bb4d000
[1]vm_free: free pte =0xffff00003bbba000
[1]vm_free: free pte =0xffff00003bbb9000
[1]vm_free: free pgt3=0xffff00003bb5d000
[1]vm_free: free pgt2=0xffff00003bb5e000
[1]vm_free: free pgt1=0xffff00003bb5f000
[1]vm_free: free pte =0xffff00003bbb8000
[1]vm_free: free pgt3=0xffff00003bb4a000
[1]vm_free: free pgt2=0xffff00003bb49000
[1]vm_free: free pgt1=0xffff00003bb4c000
[1]vm_free: free pte =0xffff00003bb4b000
[1]vm_free: free pte =0xffff00003bb42000
[1]vm_free: free pte =0xffff00003bb41000
[1]vm_free: free pte =0xffff00003bb40000
[1]vm_free: free pte =0xffff00003bb3f000
[1]vm_free: free pte =0xffff00003bb3e000
[1]vm_free: free pte =0xffff00003bb3d000
[1]vm_free: free pte =0xffff00003bb3c000
[1]vm_free: free pte =0xffff00003bb45000
[1]vm_free: free pte =0xffff00003bb46000
[1]vm_free: free pgt3=0xffff00003bb43000
[1]vm_free: free pgt2=0xffff00003bb44000
[1]vm_free: free pgt1=0xffff00003bb48000
[1]vm_free: free dir =0xffff00003bb61000
[1]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[1]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb62000, *pte=0x3bb62000
[F-12] ok

[F-13] file backed shared mapping with fork test
[0]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb62000
[1]uvm_copy: map: addr=0x400000, length=0x1000, pa=0x3bb48000
[1]uvm_copy: map: addr=0x401000, length=0x1000, pa=0x3bb45000
[1]uvm_copy: map: addr=0x402000, length=0x1000, pa=0x3bb3c000
[1]uvm_copy: map: addr=0x403000, length=0x1000, pa=0x3bb3d000
[1]uvm_copy: map: addr=0x404000, length=0x1000, pa=0x3bb3e000
[1]uvm_copy: map: addr=0x405000, length=0x1000, pa=0x3bb3f000
[1]uvm_copy: map: addr=0x406000, length=0x1000, pa=0x3bb40000
[1]uvm_copy: map: addr=0x407000, length=0x1000, pa=0x3bb41000
[1]uvm_copy: map: addr=0x408000, length=0x1000, pa=0x3bb42000
[1]uvm_copy: map: addr=0x409000, length=0x1000, pa=0x3bb4b000
[1]uvm_copy: map: addr=0x40a000, length=0x1000, pa=0x3bb4c000
[1]uvm_copy: map: addr=0x40b000, length=0x1000, pa=0x3bb49000
[1]uvm_copy: map: addr=0x40c000, length=0x1000, pa=0x3bb4a000
[1]uvm_copy: map: addr=0x40d000, length=0x1000, pa=0x3bbb8000
[1]uvm_copy: map: addr=0x40e000, length=0x1000, pa=0x3bb5f000
[1]uvm_copy: map: addr=0x40f000, length=0x1000, pa=0x3bb5e000
[1]uvm_copy: map: addr=0x410000, length=0x1000, pa=0x3bb5d000
[1]uvm_copy: map: addr=0x411000, length=0x1000, pa=0x3bbb9000
[1]uvm_copy: map: addr=0x412000, length=0x1000, pa=0x3bbba000
[1]uvm_copy: map: addr=0x600000001000, length=0x1000, pa=0x3bb4d000
[1]uvm_copy: map: addr=0xffffffff6000, length=0x1000, pa=0x3bb51000
[1]uvm_copy: map: addr=0xffffffff7000, length=0x1000, pa=0x3bb52000
[1]uvm_copy: map: addr=0xffffffff8000, length=0x1000, pa=0x3bb55000
[1]uvm_copy: map: addr=0xffffffff9000, length=0x1000, pa=0x3bb54000
[1]uvm_copy: map: addr=0xffffffffa000, length=0x1000, pa=0x3bb53000
[1]uvm_copy: map: addr=0xffffffffb000, length=0x1000, pa=0x3bb56000
[1]uvm_copy: map: addr=0xffffffffc000, length=0x1000, pa=0x3bb5a000
[1]uvm_copy: map: addr=0xffffffffd000, length=0x1000, pa=0x3bb5b000
[1]uvm_copy: map: addr=0xffffffffe000, length=0x1000, pa=0x3bb5c000
[1]uvm_copy: map: addr=0xfffffffff000, length=0x1000, pa=0x3bb60000
[1]copy_mmap_list: *ptep: 0x3bb62647, *ptec: 0x3bb4d647
[1]copy_mmap_list: unmap: addr=0x600000001000, page=1, free=1
[1]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb4d000, *pte=0x3bb4d000
[1]copy_mmap_list: map: addr=0x600000001000, length=0x1000, pa=0x3bb62000
[3]wait4: vm_free
[3]vm_free: free pte =0xffff00003bb48000
[3]vm_free: free pte =0xffff00003bb45000
[3]vm_free: free pte =0xffff00003bb3c000
[3]vm_free: free pte =0xffff00003bb3d000
[3]vm_free: free pte =0xffff00003bb3e000
[3]vm_free: free pte =0xffff00003bb3f000
[3]vm_free: free pte =0xffff00003bb40000
[3]vm_free: free pte =0xffff00003bb41000
[3]vm_free: free pte =0xffff00003bb42000
[3]vm_free: free pte =0xffff00003bb4b000
[3]vm_free: free pte =0xffff00003bb4c000
[3]vm_free: free pte =0xffff00003bb49000
[3]vm_free: free pte =0xffff00003bb4a000
[3]vm_free: free pte =0xffff00003bbb8000
[3]vm_free: free pte =0xffff00003bb5f000
[3]vm_free: free pte =0xffff00003bb5e000
[3]vm_free: free pte =0xffff00003bb5d000
[3]vm_free: free pte =0xffff00003bbb9000
[3]vm_free: free pte =0xffff00003bbba000
[3]vm_free: free pgt3=0xffff00003bb46000
[3]vm_free: free pgt2=0xffff00003bb43000
[3]vm_free: free pgt1=0xffff00003bb44000		// <= ret2[0] =0xffff00003bb44000
[3]vm_free: free pte =0xffff00003bb62000		// <= ret2(pa)=0xffff00003bb62000
[3]vm_free: free pgt3=0xffff00003bb50000
[3]vm_free: free pgt2=0xffff00003bb4f000
[3]vm_free: free pgt1=0xffff00003bb4e000
[3]vm_free: free pte =0xffff00003bb51000
[3]vm_free: free pte =0xffff00003bb52000
[3]vm_free: free pte =0xffff00003bb55000
[3]vm_free: free pte =0xffff00003bb54000
[3]vm_free: free pte =0xffff00003bb53000
[3]vm_free: free pte =0xffff00003bb56000
[3]vm_free: free pte =0xffff00003bb5a000
[3]vm_free: free pte =0xffff00003bb5b000
[3]vm_free: free pte =0xffff00003bb5c000
[3]vm_free: free pte =0xffff00003bb60000
[3]vm_free: free pgt3=0xffff00003bb2b000
[3]vm_free: free pgt2=0xffff00003bb2a000
[3]vm_free: free pgt1=0xffff00003bb29000
[3]vm_free: free dir =0xffff00003bb61000
ret2[0]=0xffff00003bb44000
[2]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[2]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb62000, *pte=0x3bb62000
[F-13] failed at strcmp 3

[F-14] オフセットを指定したプライベートマッピングのテスト
[1]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb62000
[1]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[1]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb62000, *pte=0x3bb62000
[F-14] ok

[F-15] file backed valid provided address test
[0]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb62000
[2]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[2]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb62000, *pte=0x3bb62000
[F-15] ok

[F-16] file backed invalid provided address test
[1]map_file_page: map: addr=0x5ffffffff000, length=0x1000, pa=0x3bb62000
[F-16] ok
[1]delete_mmap_node: unmap: addr=0x5ffffffff000, page=1, free=1
[1]uvm_unmap: free va=0x5ffffffff000, pa=0xffff00003bb62000, *pte=0x3bb62000

[F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合
[0]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb62000
[0]map_file_page: map: addr=0x600000002000, length=0x1000, pa=0x3bb2a000
[0]map_file_page: map: addr=0x600000003000, length=0x1000, pa=0x3bb2b000
[3]delete_mmap_node: unmap: addr=0x600000001000, page=2, free=1
[3]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb62000, *pte=0x3bb62000
[3]uvm_unmap: free va=0x600000002000, pa=0xffff00003bb2a000, *pte=0x3bb2a000
[3]delete_mmap_node: unmap: addr=0x600000003000, page=1, free=1
[3]uvm_unmap: free va=0x600000003000, pa=0xffff00003bb2b000, *pte=0x3bb2b000
[F-17] ok

[F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト
[0]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb2b000
[0]map_file_page: map: addr=0x600000003000, length=0x1000, pa=0x3bb2a000
[0]map_file_page: map: addr=0x600000002000, length=0x1000, pa=0x3bb62000
[0]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[0]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb2b000, *pte=0x3bb2b000
[0]delete_mmap_node: unmap: addr=0x600000003000, page=1, free=1
[0]uvm_unmap: free va=0x600000003000, pa=0xffff00003bb2a000, *pte=0x3bb2a000
[0]delete_mmap_node: unmap: addr=0x600000002000, page=1, free=1
[0]uvm_unmap: free va=0x600000002000, pa=0xffff00003bb62000, *pte=0x3bb62000
[F-18] ok

[F-19] ２つのマッピングの間に不可能なアドレスを指定した場合
[2]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb62000
ret =0x600000001000
[2]map_file_page: map: addr=0x600000003000, length=0x1000, pa=0x3bb2a000
ret2=0x600000003000
[2]map_file_page: map: addr=0x600000004000, length=0x1000, pa=0x3bb2b000
[2]map_file_page: map: addr=0x600000005000, length=0x1000, pa=0x3bb60000
[2]get_page: get_page readi failed: n=-1, offset=8192, size=4096
[2]copy_page: get_page failed
[2]map_file_page: map_pagecache_page: copy_page failed
ret3=0xffffffffffffffff
[F-19] failed at third mmap
[3]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[3]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb62000, *pte=0x3bb62000
[3]delete_mmap_node: unmap: addr=0x600000003000, page=1, free=1
[3]uvm_unmap: free va=0x600000003000, pa=0xffff00003bb2a000, *pte=0x3bb2a000

[F-20] 共有マッピングでファイル容量より大きなサイズを指定した場合
[1]map_file_page: map: addr=0x600000001000, length=0x1000, pa=0x3bb2a000
[1]map_file_page: map: addr=0x600000002000, length=0x1000, pa=0x3bb62000
[2]delete_mmap_node: unmap: addr=0x600000001000, page=2, free=1
[2]uvm_unmap: free va=0x600000001000, pa=0xffff00003bb2a000, *pte=0x3bb2a000
[2]uvm_unmap: free va=0x600000002000, pa=0xffff00003bb62000, *pte=0x3bb62000
[F-20] ok

[F-21] write onlyファイルへのREAD/WRITE共有マッピングのテスト
[3]sys_mmap: file is not readable
[F-21] ok

file_test:  ok: 11, ng: 2
anon_test:  ok: 0, ng: 0
other_test: ok: 0, ng: 0
[0]wait4: vm_free
[0]vm_free: free pte =0xffff00003bb64000
[0]vm_free: free pte =0xffff00003bb68000
[0]vm_free: free pte =0xffff00003bb69000
[0]vm_free: free pte =0xffff00003bb6a000
[0]vm_free: free pte =0xffff00003bb6b000
[0]vm_free: free pte =0xffff00003bb6c000
[0]vm_free: free pte =0xffff00003bb6d000
[0]vm_free: free pte =0xffff00003bb6e000
[0]vm_free: free pte =0xffff00003bb6f000
[0]vm_free: free pte =0xffff00003bb70000
[0]vm_free: free pte =0xffff00003bbfb000
[0]vm_free: free pte =0xffff00003bbc6000
[0]vm_free: free pte =0xffff00003bbc5000
[0]vm_free: free pte =0xffff00003bbc4000
[0]vm_free: free pte =0xffff00003bbc3000
[0]vm_free: free pte =0xffff00003bbc2000
[0]vm_free: free pte =0xffff00003bbc1000
[0]vm_free: free pte =0xffff00003bbc0000
[0]vm_free: free pte =0xffff00003bbbf000
[0]vm_free: free pgt3=0xffff00003bb67000
[0]vm_free: free pgt2=0xffff00003bb66000
[0]vm_free: free pgt1=0xffff00003bb65000
[0]vm_free: free pgt3=0xffff00003bb29000
[0]vm_free: free pgt2=0xffff00003bb61000
[0]vm_free: free pgt1=0xffff00003bb3b000
[0]vm_free: free pte =0xffff00003bb2b000
[0]vm_free: free pte =0xffff00003bb60000
[0]vm_free: free pgt3=0xffff00003bb59000
[0]vm_free: free pgt2=0xffff00003bb58000
[0]vm_free: free pgt1=0xffff00003bb57000
[0]vm_free: free pte =0xffff00003bbb7000
[0]vm_free: free pte =0xffff00003bb1f000
[0]vm_free: free pte =0xffff00003bb1e000
[0]vm_free: free pte =0xffff00003bb1d000
[0]vm_free: free pte =0xffff00003bb1c000
[0]vm_free: free pte =0xffff00003bb1b000
[0]vm_free: free pte =0xffff00003bb1a000
[0]vm_free: free pte =0xffff00003bb19000
[0]vm_free: free pte =0xffff00003bb18000
[0]vm_free: free pte =0xffff00003bbbe000
[0]vm_free: free pgt3=0xffff00003bbfa000
[0]vm_free: free pgt2=0xffff00003bbfe000
[0]vm_free: free pgt1=0xffff00003bbc7000
[0]vm_free: free dir =0xffff00003bb63000
[0]map_anon_page: map: addr=0x600000001000, length=0x1000, pa=0x3bbb5000
# [0]delete_mmap_node: unmap: addr=0x600000001000, page=1, free=1
[0]uvm_unmap: free va=0x600000001000, pa=0xffff00003bbb5000, *pte=0x3bbb5000
/bin/ls
[2]uvm_copy: map: addr=0x400000, length=0x1000, pa=0x3bbc7000
[2]uvm_copy: map: addr=0x401000, length=0x1000, pa=0x3bb18000
[2]uvm_copy: map: addr=0x402000, length=0x1000, pa=0x3bb19000
[2]uvm_copy: map: addr=0x403000, length=0x1000, pa=0x3bb1a000
[2]uvm_copy: map: addr=0x404000, length=0x1000, pa=0x3bb1b000
[2]uvm_copy: map: addr=0x405000, length=0x1000, pa=0x3bb1c000
[2]uvm_copy: map: addr=0x406000, length=0x1000, pa=0x3bb1d000
[2]uvm_copy: map: addr=0x407000, length=0x1000, pa=0x3bb1e000
[2]uvm_copy: map: addr=0x408000, length=0x1000, pa=0x3bb1f000
[2]uvm_copy: map: addr=0x409000, length=0x1000, pa=0x3bbb7000
[2]uvm_copy: map: addr=0x40a000, length=0x1000, pa=0x3bb57000
[2]uvm_copy: map: addr=0x40b000, length=0x1000, pa=0x3bb58000
[2]uvm_copy: map: addr=0x40c000, length=0x1000, pa=0x3bb59000
[2]uvm_copy: map: addr=0x40d000, length=0x1000, pa=0x3bb60000
[2]uvm_copy: map: addr=0x40e000, length=0x1000, pa=0x3bb2b000
[2]uvm_copy: map: addr=0x40f000, length=0x1000, pa=0x3bb3b000
[2]uvm_copy: map: addr=0x410000, length=0x1000, pa=0x3bb61000
[2]uvm_copy: map: addr=0x411000, length=0x1000, pa=0x3bb29000
[2]uvm_copy: map: addr=0x412000, length=0x1000, pa=0x3bb65000
[2]uvm_copy: map: addr=0x413000, length=0x1000, pa=0x3bb66000
[2]uvm_copy: map: addr=0x414000, length=0x1000, pa=0x3bb67000
[2]uvm_copy: map: addr=0x415000, length=0x1000, pa=0x3bbbf000
[2]uvm_copy: map: addr=0x416000, length=0x1000, pa=0x3bbc0000
[2]uvm_copy: map: addr=0x417000, length=0x1000, pa=0x3bbc1000
[2]uvm_copy: map: addr=0x418000, length=0x1000, pa=0x3bbc2000
[2]uvm_copy: map: addr=0x419000, length=0x1000, pa=0x3bbc3000
[2]uvm_copy: map: addr=0x41a000, length=0x1000, pa=0x3bbc4000
[2]uvm_copy: map: addr=0x41b000, length=0x1000, pa=0x3bbc5000
[2]uvm_copy: map: addr=0x41c000, length=0x1000, pa=0x3bbc6000
[2]uvm_copy: map: addr=0x41d000, length=0x1000, pa=0x3bbfb000
[2]uvm_copy: map: addr=0x41e000, length=0x1000, pa=0x3bb70000
[2]uvm_copy: map: addr=0x41f000, length=0x1000, pa=0x3bb6f000
[2]uvm_copy: map: addr=0x420000, length=0x1000, pa=0x3bb6e000
[2]uvm_copy: map: addr=0x421000, length=0x1000, pa=0x3bb6d000
[2]uvm_copy: map: addr=0x422000, length=0x1000, pa=0x3bb6c000
[2]uvm_copy: map: addr=0x423000, length=0x1000, pa=0x3bb6b000
[2]uvm_copy: map: addr=0x424000, length=0x1000, pa=0x3bb6a000
[2]uvm_copy: map: addr=0x425000, length=0x1000, pa=0x3bb69000
[2]uvm_copy: map: addr=0x426000, length=0x1000, pa=0x3bb68000
[2]uvm_copy: map: addr=0x427000, length=0x1000, pa=0x3bb64000
[2]uvm_copy: map: addr=0x428000, length=0x1000, pa=0x3bb62000
[2]uvm_copy: map: addr=0x429000, length=0x1000, pa=0x3bb2a000
[2]uvm_copy: map: addr=0x42a000, length=0x1000, pa=0x3bb5b000
[2]uvm_copy: map: addr=0x42b000, length=0x1000, pa=0x3bb5a000
[2]uvm_copy: map: addr=0x42c000, length=0x1000, pa=0x3bb56000
[2]uvm_copy: map: addr=0x42d000, length=0x1000, pa=0x3bb53000
[2]uvm_copy: map: addr=0x42e000, length=0x1000, pa=0x3bb54000
[2]uvm_copy: map: addr=0x42f000, length=0x1000, pa=0x3bb55000
[2]uvm_copy: map: addr=0x430000, length=0x1000, pa=0x3bb52000
[2]uvm_copy: map: addr=0x431000, length=0x1000, pa=0x3bb51000
[2]uvm_copy: map: addr=0x600000000000, length=0x1000, pa=0x3bb4e000
```

### vm_freeでret2のpaを削除していたためだった

- MAP_SHAREDの場合、fork時のuvm_copyで親のpaをそのまま子に設定するが
- wait4で子の後始末をする際に、vm_freeでMAP_SHAREDのmmap_regionのpaも削除していたのが原因だった

```
# mmaptest2

[F-13] file backed shared mapping with fork test
ret2 0x600000001000: 0x6161616161616161
pb: ret2[0]=0x6161616161616161
cb: ret2[0]=0x6161616161616161
ca: ret2[0]=0x6f6f6f6f6f6f6f6f
[3]exit: 0x6f6f6f6f6f6f6f6f
[1]wait4: (1) 0x6f6f6f6f6f6f6f6f
[1]wait4: (2) 0x6f6f6f6f6f6f6f6f		// この後、vm_free()を実行
[1]wait4: (3) 0xffff00003bb3f000		// ここでおかしな値となる
pa: ret2[0]=0xffff00003bb3f000
[F-13] ok
```

## mmaptestでも同じ現象が再現

```
# mmaptest
mmap_testスタート
[7] test mmap two files
- open mmap1 (3) RDWR/CREAT
- write 3: 12345
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000001000
- close 3
- unlink mmap1
- open mmap2 (3) RDWR/CREAT
- write 3: 67890
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000002000
- close 3
- unlink mmap2
- munmap PAGE: addr=0x600000001000
- munmap PAGE: addr=0x600000002000
[7] OK
mmap_test: Total: 1, OK: 1, NG: 0

fork_test starting
p1[PGSIZE]=A
p2[PGSIZE]=A
mismatch at 0, wanted 'A', got 0x0, addr=0x600000001000		// 同じような値
mismatch at 1, wanted 'A', got 0x80, addr=0x600000001001
mismatch at 2, wanted 'A', got 0xb1, addr=0x600000001002
mismatch at 3, wanted 'A', got 0x3b, addr=0x600000001003
mismatch at 4, wanted 'A', got 0x0, addr=0x600000001004
mismatch at 5, wanted 'A', got 0x0, addr=0x600000001005
mismatch at 6, wanted 'A', got 0xff, addr=0x600000001006
mismatch at 7, wanted 'A', got 0xff, addr=0x600000001007
- fork parent v1(p1): ret: -8
fork_test OK
mmaptest: all tests succeeded
# ls								// ストール
```

### uvm_unmap時の配慮が他にも必要だった

- munmap()
- delete_mmap_node()

```
# mmaptest
mmap_testスタート
[7] test mmap two files
- open mmap1 (3) RDWR/CREAT
- write 3: 12345
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000001000
- close 3
- unlink mmap1
- open mmap2 (3) RDWR/CREAT
- write 3: 67890
- mmap 3 -> PAGE PROT_READ MAP_PRIVATE: addr=0x600000002000
- close 3
- unlink mmap2
- munmap PAGE: addr=0x600000001000
- munmap PAGE: addr=0x600000002000
[7] OK
mmap_test: Total: 1, OK: 1, NG: 0

fork_test starting
p1: 0x600000001000 [8192]=A
p2: 0x600000003000 [16384]=A
- fork child v1(p1) ok
- fork patent v1(p1) ok
- fork patent v1(p2) ok
fork_test OK
mmaptest: all tests succeeded
# ls
bin  dev  etc  home  lib  test.txt  usr
```

## execve()のflush_old_exec()でfree_mmap_list()を実行した場合にエラー

```
$ /bin/ls
...
[1]uvm_copy: map: addr=0xfffffffff000, length=0x1000, pa=0x3bb3c000
[1]delete_mmap_node: unmap: addr=0x600000000000, page=1, free=0
[1]pgdir_walk: failed
pgdir_walk
```

- 以下の通り、free_mmap_listは不要だった

```c
void *oldpgdir = curproc->pgdir, *pgdir = vm_init();
...
curproc->pgdir = pgdir;     // この段階でcurpoc->pgdir はカラなのでメモリ関係は開放不要

flush_old_exec();   // 親から受け継いだ不要な資源を開放
```

## mmaptest2の全体テスト

- 7月9日現在、F系列で2つ失敗
- 一気通貫テストはできない（途中でストールする）
- 以下は分割してテストをした結果をまとめたもの

```
[F-01] 不正なfdを指定した場合のテスト
 error: Bad file descriptor
 error: Bad file descriptor
 error: Bad file descriptor
[F-01] ok

[F-02] 不正なフラグを指定した場合のテスト
[3]sys_mmap: invalid flags: 0x0
 error: Invalid argument
[F-02] ok

[F-03] readonlyファイルにPROT_WRITEな共有マッピングをした場合のテスト
[0]sys_mmap: file is not writable
 error: Permission denied
[F-03] ok

[F-04] readonlyファイルにPROT_READのみの共有マッピングをした場合のテスト
[F-04] ok

[F-05] PROT指定の異なるプライベートマッピンのテスト
[F-05] ok

[F-06] MMAPTOPを超えるサイズをマッピングした場合のテスト
[0]get_page: get_page readi failed: n=-1, offset=4096, size=4096
[0]copy_page: get_page failed
[0]map_file_page: map_pagecache_page: copy_page failed
 error: Out of memory
[F-06] ok

[F-07] 連続したマッピングを行うテスト
[0]uvm_map: remap: p=0x600000001000, *pte=0x3bb3c647

 error: Invalid argument
[F-07] failed at 1 total mappings

[F-08] 空のファイルを共有マッピングした場合のテスト
[F-08] ok

[F-09] ファイルが背後にあるプライベートマッピング
[F-09] ok

[F-10] ファイルが背後にある共有マッピングのテスト
[F-10] ok

[F-11] file backed mapping pagecache coherency test
[F-11] ok

[F-12] file backed private mapping with fork test
[F-12] ok

[F-13] file backed shared mapping with fork test
[F-13] ok

[F-14] オフセットを指定したプライベートマッピングのテスト
[F-14] ok

[F-15] file backed valid provided address test
[F-15] ok

[F-16] file backed invalid provided address test
[F-16] ok

[F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合
[F-17] ok

[F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト
[F-18] ok

[F-19] ２つのマッピングの間に不可能なアドレスを指定した場合
ret =0x600000001000
ret2=0x600000003000
[2]get_page: get_page readi failed: n=-1, offset=8192, size=4096
[2]copy_page: get_page failed
[2]map_file_page: map_pagecache_page: copy_page failed
ret3=0xffffffffffffffff
[F-19] failed at third mmap

[F-20] 共有マッピングでファイル容量より大きなサイズを指定した場合
[F-20] ok

[F-21] write onlyファイルへのREAD/WRITE共有マッピングのテスト
[1]sys_mmap: file is not readable
[F-21] ok

[A-01] anonymous private mapping test
p[0]=0, p[2499]=2499
[A-01] ok

[A-02] anonymous shared mapping test
[A-02] ok

[A-03] anonymous private mapping with fork test
[A-03] ok

[A-04] anonymous shared mapping with multiple forks test
[2]exit: exit: pid 12, name , err 1
[1]exit: exit: pid 11, name , err 1
[3]exit: exit: pid 10, name , err 1
[A-04] ok

[A-05] anonymous private & shared mapping together with fork test
[0]exit: exit: pid 13, name , err 1
[A-05] ok

[A-06] anonymous missing flags test
[0]sys_mmap: invalid flags: 0x20
[A-06] ok

[A-07] anonymous exceed mapping count test
[A-07] ok

[A-08] anonymous exceed mapping size test
[A-08] ok

[A-09] anonymous zero size mapping test
[0]sys_mmap: invalid length: 0 or offset: 0
[A-09] ok

[A-10] anonymous valid provided address test
[A-10] ok

[A-11] anonymous invalid provided address test
[A-11] ok: not running because of an invalid test

[A-12] anonymous overlapping provided address test
[A-12] ok

[A-13] anonymous intermediate provided address test
[A-13] ok

[A-14] anonymous intermediate provided address not possible test
[A-14] ok

[O-01] munmap only partial size test
[O-01] ok

[O-02] write on read only mapping test
[O-02] ok

[O-03] none permission on mapping test
[O-03] ok

[O-04] mmap valid address map fixed flag test
[O-04] ok

[O-05] mmap invalid address map fixed flag test
[1]mmap: fixed address should be page align: 0x600000000100
[1]mmap: addr is used
[O-05] test ok
```

### F系で失敗するのはテスト環境のせいらしい

- F-09から系全部とA系、O系の通しテストをすると全部成功
- F-01からF-08とA系、O系の通しテストはF系は全部成功するがA-01でストール
- F-01からF-07とA系、O系の通しテストはF系は全部とA=01は成功するがA-02でストール
- 個々のテストは全部OKらしいが、通しテストをストールさせる何かがあるらしい

### F-06のテスト条件が問題だった

- xv6-armv8ではMMAPTOPを600MBとしていたが今回はUSERTOPに合わせた
- この条件でF-06を実行するとmmapで想定外のエラーとなっていた
- F-06の条件の方を変えたことで一気通貫テストが成功した

```
file_test:  ok: 21, ng: 0
anon_test:  ok: 13, ng: 0
other_test: ok: 5, ng: 0
```

## MAP_SHAREDのpaは開放されることがない

- kallocが返すページにref counterを導入し
- uvm_mapで+1, uvm_unmapで-1
- 0になったらkfreeするようなロジックを考える

### 実装

- [https://github.com/vishwesh96/xv6_cow-fork](https://github.com/vishwesh96/xv6_cow-fork)のロジックを使用

```
$ make qemu
...
qemu-system-aarch64 -M raspi3b -nographic -serial null -serial mon:stdio -drive file=obj/sd.img,if=sd,format=raw -kernel obj/kernel8.img
[2]free_range: 0xffff000000212000 ~ 0xffff00003b400000, 242158 pages
[2]rand_init: rand_init ok
pagecache_init ok
[2]main: cpu 2 init finished
[0]main: cpu 0 init finished
[1]main: cpu 1 init finished
[3]main: cpu 3 init finished
[2]emmc_card_init: poweron
[2]emmc_issue_command_int: rrror occured whilst waiting for command complete interrupt
[2]emmc_card_reset: found valid version 2.00 SD card
[2]dev_init: LBA of 1st block 0x20800, 0xf0000 blocks totally
[2]iinit: sb: size 800000 nblocks 799419 ninodes 1024 nlog 126 logstart 2 inodestart 128 bmapstart 385
init: starting sh
# uname
xv6
# ls
bin  dev  etc  home  lib  test.txt  usr
# mmaptest2
...
file_test:  ok: 21, ng: 0
anon_test:  ok: 13, ng: 0
other_test: ok: 5, ng: 0
# /bin/ls
drwxrwxr-x    1 root wheel  1024  7 10 16:31 .
drwxrwxr-x    1 root wheel  1024  7 10 16:31 ..
drwxrwxr-x    2 root wheel  1024  7 10 16:31 bin
drwxrwxr-x    3 root wheel   384  7 10 16:31 dev
drwxrwxr-x    8 root wheel   256  7 10 16:31 etc
drwxrwxrwx    9 root wheel   128  7 10 16:31 lib
drwxrwxr-x   10 root wheel   192  7 10 16:31 home
drwxrwxr-x   12 root wheel   256  7 10 16:31 usr
-rwxr-xr-x   30 root wheel    94  7 10 16:31 test.txt
# sigtest
PID 22 ready
PID 23 ready
PID 24 ready
PID 25 ready
PID 26 ready
PID 26 caught sig 2, j 3
PID 25 caught sig 2, j 2
PID 24 caught sig 2, j 1
26 is dead
PID 23 caught sig 2, j 0
24 is dead
PID 22 caught sig 2, j -1
25 is dead
23 is dead
22 is dead
# uname -a
xv6 mini 1.0.1 2022-06-26 (musl) AArch64 Elf
```
