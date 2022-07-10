# 14: kallocでストール

## /bin/ls, mmaptest2, /bin/lsの順で実行すると/bin/lsでストール

```
# /bin/ls
[0]uvm_copy: i[0] pgt1=0xffff00003bbaf000
[0]uvm_copy: i1[0] pgt2=0xffff00003bbae000
[0]uvm_copy: i2[2] pgt3=0xffff00003bbad000
[0]uvm_copy: i3[0] pa=0x3bbb0000, va=0x400000
[0]uvm_copy: i3[1] pa=0x3bbac000, va=0x401000
[0]uvm_copy: i3[2] pa=0x3bbab000, va=0x402000
[0]uvm_copy: i3[3] pa=0x3bbaa000, va=0x403000
[0]uvm_copy: i3[4] pa=0x3bba9000, va=0x404000
[0]uvm_copy: i3[5] pa=0x3bba8000, va=0x405000
[0]uvm_copy: i3[6] pa=0x3bba7000, va=0x406000
[0]uvm_copy: i3[7] pa=0x3bba6000, va=0x407000
[0]uvm_copy: i3[8] pa=0x3bba5000, va=0x408000
[0]uvm_copy: i3[9] pa=0x3bba4000, va=0x409000
[0]uvm_copy: i3[10] pa=0x3bba3000, va=0x40a000
[0]uvm_copy: i3[11] pa=0x3bba2000, va=0x40b000
[0]uvm_copy: i3[12] pa=0x3bba1000, va=0x40c000
[0]uvm_copy: i3[13] pa=0x3bba0000, va=0x40d000
[0]uvm_copy: i3[14] pa=0x3bb9f000, va=0x40e000
[0]uvm_copy: i3[15] pa=0x3bb9e000, va=0x40f000
[0]uvm_copy: i3[16] pa=0x3bb9d000, va=0x410000
[0]uvm_copy: i3[17] pa=0x3bb9c000, va=0x411000
[0]uvm_copy: i3[18] pa=0x3bb9b000, va=0x412000
[0]uvm_copy: i3[19] pa=0x3bb9a000, va=0x413000
[0]uvm_copy: i3[20] pa=0x3bb99000, va=0x414000
[0]uvm_copy: i3[21] pa=0x3bb98000, va=0x415000
[0]uvm_copy: i3[22] pa=0x3bb97000, va=0x416000
[0]uvm_copy: i3[23] pa=0x3bb96000, va=0x417000
[0]uvm_copy: i3[24] pa=0x3bb95000, va=0x418000
[0]uvm_copy: i3[25] pa=0x3bb94000, va=0x419000
[0]uvm_copy: i3[26] pa=0x3bb93000, va=0x41a000
[0]uvm_copy: i3[27] pa=0x3bb92000, va=0x41b000
[0]uvm_copy: i3[28] pa=0x3bb91000, va=0x41c000
[0]uvm_copy: i3[29] pa=0x3bb90000, va=0x41d000
[0]uvm_copy: i3[30] pa=0x3bb8f000, va=0x41e000
[0]uvm_copy: i3[31] pa=0x3bb8e000, va=0x41f000
[0]uvm_copy: i3[32] pa=0x3bb8d000, va=0x420000
[0]uvm_copy: i3[33] pa=0x3bb8c000, va=0x421000
[0]uvm_copy: i3[34] pa=0x3bb8b000, va=0x422000
[0]uvm_copy: i3[35] pa=0x3bb8a000, va=0x423000
[0]uvm_copy: i3[36] pa=0x3bb89000, va=0x424000
[0]uvm_copy: i3[37] pa=0x3bb88000, va=0x425000
[0]uvm_copy: i3[38] pa=0x3bb87000, va=0x426000
[0]uvm_copy: i3[39] pa=0x3bb86000, va=0x427000
[0]uvm_copy: i3[40] pa=0x3bb85000, va=0x428000
[0]uvm_copy: i3[41] pa=0x3bb84000, va=0x429000
[0]uvm_copy: i3[42] pa=0x3bb83000, va=0x42a000
[0]uvm_copy: i3[43] pa=0x3bb82000, va=0x42b000
[0]uvm_copy: i3[44] pa=0x3bb81000, va=0x42c000
[0]uvm_copy: i3[45] pa=0x3bb80000, va=0x42d000
[0]uvm_copy: i3[46] pa=0x3bb7f000, va=0x42e000
[0]uvm_copy: i3[47] pa=0x3bb7e000, va=0x42f000
[0]uvm_copy: i3[48] pa=0x3bbfc000, va=0x430000
[0]uvm_copy: i3[49] pa=0x3bbbd000, va=0x431000
[0]uvm_copy: i[192] pgt1=0xffff00003bbb2000
[0]uvm_copy: i1[0] pgt2=0xffff00003bbb3000
[0]uvm_copy: i2[0] pgt3=0xffff00003bbb4000
[0]uvm_copy: i3[0] pa=0x3bbbb000, va=0x600000000000
[0]uvm_copy: i[511] pgt1=0xffff00003bb7d000
[0]uvm_copy: i1[511] pgt2=0xffff00003bb7c000
[0]uvm_copy: i2[511] pgt3=0xffff00003bb7b000
[0]uvm_copy: i3[502] pa=0x3bb79000, va=0xffffffff6000
[0]uvm_copy: i3[503] pa=0x3bb78000, va=0xffffffff7000
[0]uvm_copy: i3[504] pa=0x3bb77000, va=0xffffffff8000
[0]uvm_copy: i3[505] pa=0x3bb76000, va=0xffffffff9000
[0]uvm_copy: i3[506] pa=0x3bb75000, va=0xffffffffa000
[0]uvm_copy: i3[507] pa=0x3bb74000, va=0xffffffffb000
[0]uvm_copy: i3[508] pa=0x3bb73000, va=0xffffffffc000
[0]uvm_copy: i3[509] pa=0x3bb72000, va=0xffffffffd000
[0]uvm_copy: i3[510] pa=0x3bb71000, va=0xffffffffe000
[0]uvm_copy: i3[511] pa=0x3bb7a000, va=0xfffffffff000
drwxrwxr-x    1 root wheel  1024  7  9 09:03 .
drwxrwxr-x    1 root wheel  1024  7  9 09:03 ..
drwxrwxr-x    2 root wheel  1024  7  9 09:03 bin
drwxrwxr-x    3 root wheel   384  7  9 09:03 dev
drwxrwxr-x    8 root wheel   256  7  9 09:03 etc
drwxrwxrwx    9 root wheel   128  7  9 09:03 lib
drwxrwxr-x   10 root wheel   192  7  9 09:03 home
drwxrwxr-x   12 root wheel   256  7  9 09:03 usr
-rwxr-xr-x   30 root wheel    94  7  9 09:03 test.txt
# mmaptest2
[3]uvm_copy: i[0] pgt1=0xffff00003bbaf000
[3]uvm_copy: i1[0] pgt2=0xffff00003bbae000
[3]uvm_copy: i2[2] pgt3=0xffff00003bbad000
[3]uvm_copy: i3[0] pa=0x3bbb0000, va=0x400000
[3]uvm_copy: i3[1] pa=0x3bbac000, va=0x401000
[3]uvm_copy: i3[2] pa=0x3bbab000, va=0x402000
[3]uvm_copy: i3[3] pa=0x3bbaa000, va=0x403000
[3]uvm_copy: i3[4] pa=0x3bba9000, va=0x404000
[3]uvm_copy: i3[5] pa=0x3bba8000, va=0x405000
[3]uvm_copy: i3[6] pa=0x3bba7000, va=0x406000
[3]uvm_copy: i3[7] pa=0x3bba6000, va=0x407000
[3]uvm_copy: i3[8] pa=0x3bba5000, va=0x408000
[3]uvm_copy: i3[9] pa=0x3bba4000, va=0x409000
[3]uvm_copy: i3[10] pa=0x3bba3000, va=0x40a000
[3]uvm_copy: i3[11] pa=0x3bba2000, va=0x40b000
[3]uvm_copy: i3[12] pa=0x3bba1000, va=0x40c000
[3]uvm_copy: i3[13] pa=0x3bba0000, va=0x40d000
[3]uvm_copy: i3[14] pa=0x3bb9f000, va=0x40e000
[3]uvm_copy: i3[15] pa=0x3bb9e000, va=0x40f000
[3]uvm_copy: i3[16] pa=0x3bb9d000, va=0x410000
[3]uvm_copy: i3[17] pa=0x3bb9c000, va=0x411000
[3]uvm_copy: i3[18] pa=0x3bb9b000, va=0x412000
[3]uvm_copy: i3[19] pa=0x3bb9a000, va=0x413000
[3]uvm_copy: i3[20] pa=0x3bb99000, va=0x414000
[3]uvm_copy: i3[21] pa=0x3bb98000, va=0x415000
[3]uvm_copy: i3[22] pa=0x3bb97000, va=0x416000
[3]uvm_copy: i3[23] pa=0x3bb96000, va=0x417000
[3]uvm_copy: i3[24] pa=0x3bb95000, va=0x418000
[3]uvm_copy: i3[25] pa=0x3bb94000, va=0x419000
[3]uvm_copy: i3[26] pa=0x3bb93000, va=0x41a000
[3]uvm_copy: i3[27] pa=0x3bb92000, va=0x41b000
[3]uvm_copy: i3[28] pa=0x3bb91000, va=0x41c000
[3]uvm_copy: i3[29] pa=0x3bb90000, va=0x41d000
[3]uvm_copy: i3[30] pa=0x3bb8f000, va=0x41e000
[3]uvm_copy: i3[31] pa=0x3bb8e000, va=0x41f000
[3]uvm_copy: i3[32] pa=0x3bb8d000, va=0x420000
[3]uvm_copy: i3[33] pa=0x3bb8c000, va=0x421000
[3]uvm_copy: i3[34] pa=0x3bb8b000, va=0x422000
[3]uvm_copy: i3[35] pa=0x3bb8a000, va=0x423000
[3]uvm_copy: i3[36] pa=0x3bb89000, va=0x424000
[3]uvm_copy: i3[37] pa=0x3bb88000, va=0x425000
[3]uvm_copy: i3[38] pa=0x3bb87000, va=0x426000
[3]uvm_copy: i3[39] pa=0x3bb86000, va=0x427000
[3]uvm_copy: i3[40] pa=0x3bb85000, va=0x428000
[3]uvm_copy: i3[41] pa=0x3bb84000, va=0x429000
[3]uvm_copy: i3[42] pa=0x3bb83000, va=0x42a000
[3]uvm_copy: i3[43] pa=0x3bb82000, va=0x42b000
[3]uvm_copy: i3[44] pa=0x3bb81000, va=0x42c000
[3]uvm_copy: i3[45] pa=0x3bb80000, va=0x42d000
[3]uvm_copy: i3[46] pa=0x3bb7f000, va=0x42e000
[3]uvm_copy: i3[47] pa=0x3bb7e000, va=0x42f000
[3]uvm_copy: i3[48] pa=0x3bbfc000, va=0x430000
[3]uvm_copy: i3[49] pa=0x3bbbd000, va=0x431000
[3]uvm_copy: i[192] pgt1=0xffff00003bbb2000
[3]uvm_copy: i1[0] pgt2=0xffff00003bbb3000
[3]uvm_copy: i2[0] pgt3=0xffff00003bbb4000
[3]uvm_copy: i3[0] pa=0x3bbbb000, va=0x600000000000
[3]uvm_copy: i[511] pgt1=0xffff00003bb7d000
[3]uvm_copy: i1[511] pgt2=0xffff00003bb7c000
[3]uvm_copy: i2[511] pgt3=0xffff00003bb7b000
[3]uvm_copy: i3[502] pa=0x3bb79000, va=0xffffffff6000
[3]uvm_copy: i3[503] pa=0x3bb78000, va=0xffffffff7000
[3]uvm_copy: i3[504] pa=0x3bb77000, va=0xffffffff8000
[3]uvm_copy: i3[505] pa=0x3bb76000, va=0xffffffff9000
[3]uvm_copy: i3[506] pa=0x3bb75000, va=0xffffffffa000
[3]uvm_copy: i3[507] pa=0x3bb74000, va=0xffffffffb000
[3]uvm_copy: i3[508] pa=0x3bb73000, va=0xffffffffc000
[3]uvm_copy: i3[509] pa=0x3bb72000, va=0xffffffffd000
[3]uvm_copy: i3[510] pa=0x3bb71000, va=0xffffffffe000
[3]uvm_copy: i3[511] pa=0x3bb7a000, va=0xfffffffff000

[F-09] ファイルが背後にあるプライベートマッピング
[F-09] ok

[F-10] ファイルが背後にある共有マッピングのテスト
[F-10] ok

[F-11] file backed mapping pagecache coherency test
[F-11] ok

[F-12] file backed private mapping with fork test
[3]uvm_copy: i[0] pgt1=0xffff00003bb65000
[3]uvm_copy: i1[0] pgt2=0xffff00003bb66000
[3]uvm_copy: i2[2] pgt3=0xffff00003bb67000
[3]uvm_copy: i3[0] pa=0x3bb64000, va=0x400000
[3]uvm_copy: i3[1] pa=0x3bb68000, va=0x401000
[3]uvm_copy: i3[2] pa=0x3bb69000, va=0x402000
[3]uvm_copy: i3[3] pa=0x3bb6a000, va=0x403000
[3]uvm_copy: i3[4] pa=0x3bb6b000, va=0x404000
[3]uvm_copy: i3[5] pa=0x3bb6c000, va=0x405000
[3]uvm_copy: i3[6] pa=0x3bb6d000, va=0x406000
[3]uvm_copy: i3[7] pa=0x3bb6e000, va=0x407000
[3]uvm_copy: i3[8] pa=0x3bb6f000, va=0x408000
[3]uvm_copy: i3[9] pa=0x3bb70000, va=0x409000
[3]uvm_copy: i3[10] pa=0x3bbfb000, va=0x40a000
[3]uvm_copy: i3[11] pa=0x3bbc6000, va=0x40b000
[3]uvm_copy: i3[12] pa=0x3bbc5000, va=0x40c000
[3]uvm_copy: i3[13] pa=0x3bbc4000, va=0x40d000
[3]uvm_copy: i3[14] pa=0x3bbc3000, va=0x40e000
[3]uvm_copy: i3[15] pa=0x3bbc2000, va=0x40f000
[3]uvm_copy: i3[16] pa=0x3bbc1000, va=0x410000
[3]uvm_copy: i3[17] pa=0x3bbc0000, va=0x411000
[3]uvm_copy: i3[18] pa=0x3bbbf000, va=0x412000
[3]uvm_copy: i[192] pgt1=0xffff00003bb57000
[3]uvm_copy: i1[0] pgt2=0xffff00003bb58000
[3]uvm_copy: i2[0] pgt3=0xffff00003bb59000
[3]uvm_copy: i3[1] pa=0x3bb62000, va=0x600000001000
[3]uvm_copy: i[511] pgt1=0xffff00003bbc7000
[3]uvm_copy: i1[511] pgt2=0xffff00003bbfe000
[3]uvm_copy: i2[511] pgt3=0xffff00003bbfa000
[3]uvm_copy: i3[502] pa=0x3bbb7000, va=0xffffffff6000
[3]uvm_copy: i3[503] pa=0x3bb1f000, va=0xffffffff7000
[3]uvm_copy: i3[504] pa=0x3bb1e000, va=0xffffffff8000
[3]uvm_copy: i3[505] pa=0x3bb1d000, va=0xffffffff9000
[3]uvm_copy: i3[506] pa=0x3bb1c000, va=0xffffffffa000
[3]uvm_copy: i3[507] pa=0x3bb1b000, va=0xffffffffb000
[3]uvm_copy: i3[508] pa=0x3bb1a000, va=0xffffffffc000
[3]uvm_copy: i3[509] pa=0x3bb19000, va=0xffffffffd000
[3]uvm_copy: i3[510] pa=0x3bb18000, va=0xffffffffe000
[3]uvm_copy: i3[511] pa=0x3bbbe000, va=0xfffffffff000
[F-12] ok

[F-13] file backed shared mapping with fork test
[0]uvm_copy: i[0] pgt1=0xffff00003bb65000
[0]uvm_copy: i1[0] pgt2=0xffff00003bb66000
[0]uvm_copy: i2[2] pgt3=0xffff00003bb67000
[0]uvm_copy: i3[0] pa=0x3bb64000, va=0x400000
[0]uvm_copy: i3[1] pa=0x3bb68000, va=0x401000
[0]uvm_copy: i3[2] pa=0x3bb69000, va=0x402000
[0]uvm_copy: i3[3] pa=0x3bb6a000, va=0x403000
[0]uvm_copy: i3[4] pa=0x3bb6b000, va=0x404000
[0]uvm_copy: i3[5] pa=0x3bb6c000, va=0x405000
[0]uvm_copy: i3[6] pa=0x3bb6d000, va=0x406000
[0]uvm_copy: i3[7] pa=0x3bb6e000, va=0x407000
[0]uvm_copy: i3[8] pa=0x3bb6f000, va=0x408000
[0]uvm_copy: i3[9] pa=0x3bb70000, va=0x409000
[0]uvm_copy: i3[10] pa=0x3bbfb000, va=0x40a000
[0]uvm_copy: i3[11] pa=0x3bbc6000, va=0x40b000
[0]uvm_copy: i3[12] pa=0x3bbc5000, va=0x40c000
[0]uvm_copy: i3[13] pa=0x3bbc4000, va=0x40d000
[0]uvm_copy: i3[14] pa=0x3bbc3000, va=0x40e000
[0]uvm_copy: i3[15] pa=0x3bbc2000, va=0x40f000
[0]uvm_copy: i3[16] pa=0x3bbc1000, va=0x410000
[0]uvm_copy: i3[17] pa=0x3bbc0000, va=0x411000
[0]uvm_copy: i3[18] pa=0x3bbbf000, va=0x412000
[0]uvm_copy: i[192] pgt1=0xffff00003bb57000
[0]uvm_copy: i1[0] pgt2=0xffff00003bb58000
[0]uvm_copy: i2[0] pgt3=0xffff00003bb59000
[0]uvm_copy: i3[1] pa=0x3bb62000, va=0x600000001000
[0]uvm_copy: i[511] pgt1=0xffff00003bbc7000
[0]uvm_copy: i1[511] pgt2=0xffff00003bbfe000
[0]uvm_copy: i2[511] pgt3=0xffff00003bbfa000
[0]uvm_copy: i3[502] pa=0x3bbb7000, va=0xffffffff6000
[0]uvm_copy: i3[503] pa=0x3bb1f000, va=0xffffffff7000
[0]uvm_copy: i3[504] pa=0x3bb1e000, va=0xffffffff8000
[0]uvm_copy: i3[505] pa=0x3bb1d000, va=0xffffffff9000
[0]uvm_copy: i3[506] pa=0x3bb1c000, va=0xffffffffa000
[0]uvm_copy: i3[507] pa=0x3bb1b000, va=0xffffffffb000
[0]uvm_copy: i3[508] pa=0x3bb1a000, va=0xffffffffc000
[0]uvm_copy: i3[509] pa=0x3bb19000, va=0xffffffffd000
[0]uvm_copy: i3[510] pa=0x3bb18000, va=0xffffffffe000
[0]uvm_copy: i3[511] pa=0x3bbbe000, va=0xfffffffff000
ret2[0]=0xffff00003bb44000
[F-13] failed at strcmp 3

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
[3]get_page: get_page readi failed: n=-1, offset=8192, size=4096
[3]copy_page: get_page failed
[3]map_file_page: map_pagecache_page: copy_page failed
ret3=0xffffffffffffffff
[F-19] failed at third mmap

[F-20] 共有マッピングでファイル容量より大きなサイズを指定した場合
[F-20] ok

[F-21] write onlyファイルへのREAD/WRITE共有マッピングのテスト
[0]sys_mmap: file is not readable
[F-21] ok

file_test:  ok: 11, ng: 2
anon_test:  ok: 0, ng: 0
other_test: ok: 0, ng: 0
# /bin/ls
[1]uvm_copy: i[0] pgt1=0xffff00003bbaf000
[1]uvm_copy: i1[0] pgt2=0xffff00003bbae000
[1]uvm_copy: i2[2] pgt3=0xffff00003bbad000
[1]uvm_copy: i3[0] pa=0x3bbb0000, va=0x400000
[1]uvm_copy: i3[1] pa=0x3bbac000, va=0x401000
[1]uvm_copy: i3[2] pa=0x3bbab000, va=0x402000
[1]uvm_copy: i3[3] pa=0x3bbaa000, va=0x403000
[1]uvm_copy: i3[4] pa=0x3bba9000, va=0x404000
[1]uvm_copy: i3[5] pa=0x3bba8000, va=0x405000
[1]uvm_copy: i3[6] pa=0x3bba7000, va=0x406000
[1]uvm_copy: i3[7] pa=0x3bba6000, va=0x407000
[1]uvm_copy: i3[8] pa=0x3bba5000, va=0x408000
[1]uvm_copy: i3[9] pa=0x3bba4000, va=0x409000
[1]uvm_copy: i3[10] pa=0x3bba3000, va=0x40a000
[1]uvm_copy: i3[11] pa=0x3bba2000, va=0x40b000
[1]uvm_copy: i3[12] pa=0x3bba1000, va=0x40c000
[1]uvm_copy: i3[13] pa=0x3bba0000, va=0x40d000
[1]uvm_copy: i3[14] pa=0x3bb9f000, va=0x40e000
[1]uvm_copy: i3[15] pa=0x3bb9e000, va=0x40f000
[1]uvm_copy: i3[16] pa=0x3bb9d000, va=0x410000
[1]uvm_copy: i3[17] pa=0x3bb9c000, va=0x411000
[1]uvm_copy: i3[18] pa=0x3bb9b000, va=0x412000
[1]uvm_copy: i3[19] pa=0x3bb9a000, va=0x413000
[1]uvm_copy: i3[20] pa=0x3bb99000, va=0x414000
[1]uvm_copy: i3[21] pa=0x3bb98000, va=0x415000
[1]uvm_copy: i3[22] pa=0x3bb97000, va=0x416000
[1]uvm_copy: i3[23] pa=0x3bb96000, va=0x417000
[1]uvm_copy: i3[24] pa=0x3bb95000, va=0x418000
[1]uvm_copy: i3[25] pa=0x3bb94000, va=0x419000
[1]uvm_copy: i3[26] pa=0x3bb93000, va=0x41a000
[1]uvm_copy: i3[27] pa=0x3bb92000, va=0x41b000
[1]uvm_copy: i3[28] pa=0x3bb91000, va=0x41c000
[1]uvm_copy: i3[29] pa=0x3bb90000, va=0x41d000
[1]uvm_copy: i3[30] pa=0x3bb8f000, va=0x41e000
[1]uvm_copy: i3[31] pa=0x3bb8e000, va=0x41f000
[1]uvm_copy: i3[32] pa=0x3bb8d000, va=0x420000
[1]uvm_copy: i3[33] pa=0x3bb8c000, va=0x421000
[1]uvm_copy: i3[34] pa=0x3bb8b000, va=0x422000
[1]uvm_copy: i3[35] pa=0x3bb8a000, va=0x423000
[1]uvm_copy: i3[36] pa=0x3bb89000, va=0x424000
[1]uvm_copy: i3[37] pa=0x3bb88000, va=0x425000
[1]uvm_copy: i3[38] pa=0x3bb87000, va=0x426000
[1]uvm_copy: i3[39] pa=0x3bb86000, va=0x427000
[1]uvm_copy: i3[40] pa=0x3bb85000, va=0x428000
[1]uvm_copy: i3[41] pa=0x3bb84000, va=0x429000
[1]uvm_copy: i3[42] pa=0x3bb83000, va=0x42a000
[1]uvm_copy: i3[43] pa=0x3bb82000, va=0x42b000
[1]uvm_copy: i3[44] pa=0x3bb81000, va=0x42c000
[1]uvm_copy: i3[45] pa=0x3bb80000, va=0x42d000
[1]uvm_copy: i3[46] pa=0x3bb7f000, va=0x42e000
[1]uvm_copy: i3[47] pa=0x3bb7e000, va=0x42f000
[1]uvm_copy: i3[48] pa=0x3bbfc000, va=0x430000
[1]uvm_copy: i3[49] pa=0x3bbbd000, va=0x431000
[1]uvm_copy: i[192] pgt1=0xffff00003bbb2000
[1]uvm_copy: i1[0] pgt2=0xffff00003bbb3000
[1]uvm_copy: i2[0] pgt3=0xffff00003bbb4000
[1]uvm_copy: i3[0] pa=0x3bbbb000, va=0x600000000000
[1]uvm_copy: i[511] pgt1=0xffff00003bb7d000
[1]uvm_copy: i1[511] pgt2=0xffff00003bb7c000
[1]uvm_copy: i2[511] pgt3=0xffff00003bb7b000
[1]uvm_copy: i3[502] pa=0x3bb79000, va=0xffffffff6000
```

## ストールしているのはkalloc()だった

- freelist_alloc()関数のf->nextでおかしな値が返るため

```
# 正常な場合

(gdb) s
freelist_alloc (f=0xffff0000000e0db0 <freelist>) at kern/mm.c:40
40	    void *p = f->next;
(gdb)
41	    if (p)
(gdb) p/x p
$7 = 0xffff00003bb51000
(gdb) p/x *(void **)p
$8 = 0xffff00003bb29000
(gdb) c
Continuing.

# ストールの場合

freelist_alloc (f=0xffff0000000e0db0 <freelist>) at kern/mm.c:40
40	    void *p = f->next;
(gdb)
41	    if (p)
(gdb) p/x p
$9 = 0x9e670000b340bca3
(gdb) p/x *(void **)p
Cannot access memory at address 0x9e670000b340bca3
```

## break vm.c:79 if (i==511 && i1==511 && i2==511 && i3==502)でデバッグ

```
Breakpoint 1 at 0xffff000000086d08: file kern/vm.c, line 79.
Continuing.
[Switching to Thread 1.2]
```

### sh

```
Thread 2 hit Breakpoint 1, uvm_copy (pgdir=0xffff00003bbe1000) at kern/vm.c:79
79	                                if (pgt3[i3] & PTE_VALID) {
Continuing.
[Switching to Thread 1.3]

Thread 3 hit Breakpoint 1, uvm_copy (pgdir=0xffff00003bbb1000) at kern/vm.c:79
79	                                if (pgt3[i3] & PTE_VALID) {
81	                                    assert(pgt3[i3] & PTE_PAGE);
83	                                    assert(pgt3[i3] & PTE_NORMAL);
85	                                    assert(PTE_ADDR(pgt3[i3]) < KERNBASE);
87	                                    uint64_t pa = PTE_ADDR(pgt3[i3]);
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
89	                                                | (uint64_t) i1 << L1SHIFT
90	                                                | (uint64_t) i2 << L2SHIFT
91	                                                | (uint64_t) i3 << L3SHIFT;
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
93	                                    void *np = kalloc();
kalloc () at kern/mm.c:95
95	    acquire(&memlock);
96	    void *p = freelist_alloc(&freelist);
freelist_alloc (f=0xffff0000000e0db0 <freelist>) at kern/mm.c:40
40	    void *p = f->next;
41	    if (p)
(gdb) p/x f
$1 = 0xffff0000000e0db0
(gdb) x/-16g f
0xffff0000000e0d30 <bcache+72472>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d40 <bcache+72488>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d50 <bcache+72504>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d60 <bcache+72520>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d70 <bcache+72536>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d80 <bcache+72552>:	0xffff0000000e0da0	0xffff0000000e0b40
0xffff0000000e0d90 <bcache+72568>:	0xffff000000118000	0xffff000000118000
0xffff0000000e0da0 <bcache+72584>:	0xffff0000000cf440	0xffff0000000e0d80
(gdb) x/16g f
0xffff0000000e0db0 <freelist>:	0xffff00003bb48000	0x0000000000000000
0xffff0000000e0dc0 <freelist+16>:	0x0000000000000000	0x0000000000000001
0xffff0000000e0dd0 <totalram>:	0x000000003bedb000	0xffff00000007ff50
0xffff0000000e0de0 <cpu+8>:	0xffff0000000e26d8	0xffff0000000e26d8
0xffff0000000e0df0 <cpu+24>:	0x0000000000000000	0xffff00000007ef50
0xffff0000000e0e00 <cpu+40>:	0xffff0000000e20b8	0xffff0000000e20b8
0xffff0000000e0e10 <cpu+56>:	0x0000000000000000	0xffff00000007df50
0xffff0000000e0e20 <cpu+72>:	0xffff0000000e2cf8	0xffff0000000e1a98
(gdb) p/x p
$2 = 0xffff00003bb48000
(gdb) p/x *(struct freelist *)p
$3 = {
  next = 0xffff00003bb47000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)f
$4 = {
  next = 0xffff00003bb48000,
  start = 0x0,
  end = 0x0
}
(gdb) x/-16gx p
0xffff00003bb47f80:	0x0000000000000000	0x0000000000000000
0xffff00003bb47f90:	0x0000000000000000	0x0000000000000000
0xffff00003bb47fa0:	0x0000000000000000	0x0000000000000000
0xffff00003bb47fb0:	0x0000000000000000	0x0000000000000000
0xffff00003bb47fc0:	0x0000000000000000	0x0000000000000000
0xffff00003bb47fd0:	0x0000000000000000	0x0000000000000000
0xffff00003bb47fe0:	0x0000000000000000	0x0000000000000000
0xffff00003bb47ff0:	0x0000000000000000	0x0000000000000000
(gdb) x/16gx p
0xffff00003bb48000:	0xffff00003bb47000	0x0000000000000000
0xffff00003bb48010:	0x0000000000000000	0x0000000000000000
0xffff00003bb48020:	0x0000000000000000	0x0000000000000000
0xffff00003bb48030:	0x0000000000000000	0x0000000000000000
0xffff00003bb48040:	0x0000000000000000	0x0000000000000000
0xffff00003bb48050:	0x0000000000000000	0x0000000000000000
0xffff00003bb48060:	0x0000000000000000	0x0000000000000000
0xffff00003bb48070:	0x0000000000000000	0x0000000000000000
(gdb) p/x *(struct freelist *)0xffff00003bb47000
$5 = {
  next = 0xffff00003bb46000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff00003bb46000
$6 = {
  next = 0xffff00003bb45000,
  start = 0x0,
  end = 0x0
}
(gdb) c
Continuing.
```

### /bin/ls

```
[Switching to Thread 1.4]

Thread 4 hit Breakpoint 1, uvm_copy (pgdir=0xffff00003bbb1000) at kern/vm.c:79
79	                                if (pgt3[i3] & PTE_VALID) {
81	                                    assert(pgt3[i3] & PTE_PAGE);
83	                                    assert(pgt3[i3] & PTE_NORMAL);
85	                                    assert(PTE_ADDR(pgt3[i3]) < KERNBASE);
87	                                    uint64_t pa = PTE_ADDR(pgt3[i3]);
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
89	                                                | (uint64_t) i1 << L1SHIFT
90	                                                | (uint64_t) i2 << L2SHIFT
91	                                                | (uint64_t) i3 << L3SHIFT;
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
93	                                    void *np = kalloc();
kalloc () at kern/mm.c:95
95	    acquire(&memlock);
96	    void *p = freelist_alloc(&freelist);
freelist_alloc (f=0xffff0000000e0db0 <freelist>) at kern/mm.c:40
40	    void *p = f->next;
41	    if (p)
(gdb) p/x f
$7 = 0xffff0000000e0db0
(gdb) p/x *(struct freelist *)f
$8 = {
  next = 0xffff00003bb56000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x p
$9 = 0xffff00003bb56000
(gdb) x/-16gx f
0xffff0000000e0d30 <bcache+72472>:	0x00000000079690e0	0x0000060e0000060d
0xffff0000000e0d40 <bcache+72488>:	0x000006100000060f	0x0000061200000611
0xffff0000000e0d50 <bcache+72504>:	0x0000061400000613	0x0000061600000615
0xffff0000000e0d60 <bcache+72520>:	0x0000061800000617	0x0000000000000000
0xffff0000000e0d70 <bcache+72536>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d80 <bcache+72552>:	0xffff0000000e0da0	0xffff0000000e0b40
0xffff0000000e0d90 <bcache+72568>:	0xffff000000118000	0xffff000000118000
0xffff0000000e0da0 <bcache+72584>:	0xffff0000000cf440	0xffff0000000e0d80
(gdb) x/16gx f
0xffff0000000e0db0 <freelist>:	0xffff00003bb56000	0x0000000000000000
0xffff0000000e0dc0 <freelist+16>:	0x0000000000000000	0x0000000000000001
0xffff0000000e0dd0 <totalram>:	0x000000003bedb000	0xffff00000007ff50
0xffff0000000e0de0 <cpu+8>:	0xffff0000000e26d8	0xffff0000000e26d8
0xffff0000000e0df0 <cpu+24>:	0x0000000000000000	0xffff00000007ef50
0xffff0000000e0e00 <cpu+40>:	0xffff0000000e20b8	0xffff0000000e20b8
0xffff0000000e0e10 <cpu+56>:	0x0000000000000000	0xffff00000007df50
0xffff0000000e0e20 <cpu+72>:	0xffff0000000e1a98	0xffff0000000e1a98
(gdb) p/x *(struct freelist *)p
$10 = {
  next = 0xffff00003bb57000,
  start = 0xb3410083b3503823,
  end = 0xd65f03c09eaf0060
}
(gdb) p/x *(struct freelist *)0xffff00003bb56000
$11 = {
  next = 0xffff00003bb57000,
  start = 0xb3410083b3503823,
  end = 0xd65f03c09eaf0060
}
(gdb) p/x *(struct freelist *)0xffff00003bb57000
$12 = {
  next = 0xffff00003bb58000,
  start = 0x17ffff6eb79ffa84,
  end = 0x17ffffd1321c0000
}
(gdb) c
Continuing.
```

### mmaptest2

```
[Switching to Thread 1.1]

Thread 1 hit Breakpoint 1, uvm_copy (pgdir=0xffff00003bb63000) at kern/vm.c:79
79	                                if (pgt3[i3] & PTE_VALID) {
81	                                    assert(pgt3[i3] & PTE_PAGE);
83	                                    assert(pgt3[i3] & PTE_NORMAL);
85	                                    assert(PTE_ADDR(pgt3[i3]) < KERNBASE);
87	                                    uint64_t pa = PTE_ADDR(pgt3[i3]);
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
89	                                                | (uint64_t) i1 << L1SHIFT
90	                                                | (uint64_t) i2 << L2SHIFT
91	                                                | (uint64_t) i3 << L3SHIFT;
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
93	                                    void *np = kalloc();
kalloc () at kern/mm.c:95
95	    acquire(&memlock);
96	    void *p = freelist_alloc(&freelist);
freelist_alloc (f=0xffff0000000e0db0 <freelist>) at kern/mm.c:40
40	    void *p = f->next;
41	    if (p)
(gdb) p/x p
$13 = 0xffff00003bb4b000
(gdb) p/x f
$14 = 0xffff0000000e0db0
(gdb) x/-16gx f
0xffff0000000e0d30 <bcache+72472>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d40 <bcache+72488>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d50 <bcache+72504>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d60 <bcache+72520>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d70 <bcache+72536>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d80 <bcache+72552>:	0xffff0000000e0480	0xffff0000000e0900
0xffff0000000e0d90 <bcache+72568>:	0xffff000000118000	0xffff000000118000
(gdb) x/16gx f
0xffff0000000e0da0 <bcache+72584>:	0xffff0000000cf440	0xffff0000000e06c0
0xffff0000000e0db0 <freelist>:	0xffff00003bb4b000	0x0000000000000000
0xffff0000000e0dc0 <freelist+16>:	0x0000000000000000	0x0000000000000001
0xffff0000000e0dd0 <totalram>:	0x000000003bedb000	0xffff00000007ff50
0xffff0000000e0de0 <cpu+8>:	0xffff0000000e3318	0xffff0000000e26d8
0xffff0000000e0df0 <cpu+24>:	0x0000000000000000	0xffff00000007ef50
0xffff0000000e0e00 <cpu+40>:	0xffff0000000e20b8	0xffff0000000e20b8
0xffff0000000e0e10 <cpu+56>:	0x0000000000000000	0xffff00000007df50
0xffff0000000e0e20 <cpu+72>:	0xffff0000000e1a98	0xffff0000000e1a98
(gdb) p/x *(struct freelist *)p
$15 = {
  next = 0xffff00003bb48000,
  start = 0xaa0503e717ffff3a,
  end = 0xaa0a03e9aa0103e6
}
(gdb) p/x *(struct freelist *)f
$16 = {
  next = 0xffff00003bb4b000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff00003bb4b000
$17 = {
  next = 0xffff00003bb48000,
  start = 0xaa0503e717ffff3a,
  end = 0xaa0a03e9aa0103e6
}
(gdb) p/x *(struct freelist *)0xffff00003bb48000
$18 = {
  next = 0xffff00003bb44000,
  start = 0x54000a8d7100001f,
  end = 0xb24d0063b4000f07
}
(gdb) p/x *(struct freelist *)0xffff00003bb44000
$19 = {
  next = 0xffff00003bb43000,
  start = 0xd280000254000141,
  end = 0xaa1403e3aa1303e4
}
(gdb) c
Continuing.
[Switching to Thread 1.4]
```

### /bin/ls

```
Thread 4 hit Breakpoint 1, uvm_copy (pgdir=0xffff00003bb63000) at kern/vm.c:79
79	                                if (pgt3[i3] & PTE_VALID) {
Continuing.
[Switching to Thread 1.1]

Thread 1 hit Breakpoint 1, uvm_copy (pgdir=0xffff00003bbb1000) at kern/vm.c:79
79	                                if (pgt3[i3] & PTE_VALID) {
81	                                    assert(pgt3[i3] & PTE_PAGE);
83	                                    assert(pgt3[i3] & PTE_NORMAL);
85	                                    assert(PTE_ADDR(pgt3[i3]) < KERNBASE);
87	                                    uint64_t pa = PTE_ADDR(pgt3[i3]);
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
89	                                                | (uint64_t) i1 << L1SHIFT
90	                                                | (uint64_t) i2 << L2SHIFT
91	                                                | (uint64_t) i3 << L3SHIFT;
88	                                    uint64_t va = (uint64_t) i << L0SHIFT
93	                                    void *np = kalloc();
kalloc () at kern/mm.c:95
95	    acquire(&memlock);
96	    void *p = freelist_alloc(&freelist);
freelist_alloc (f=0xffff0000000e0db0 <freelist>) at kern/mm.c:40
40	    void *p = f->next;
41	    if (p)
(gdb) p/x p
$20 = 0x9e670000b340bca3
(gdb) p/x f
$21 = 0xffff0000000e0db0
(gdb) p/x *(struct freelist *)f
$22 = {
  next = 0x9e670000b340bca3,
  start = 0x0,
  end = 0x0
}
(gdb) x/-16gx f
0xffff0000000e0d30 <bcache+72472>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d40 <bcache+72488>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d50 <bcache+72504>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d60 <bcache+72520>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d70 <bcache+72536>:	0x0000000000000000	0x0000000000000000
0xffff0000000e0d80 <bcache+72552>:	0xffff0000000e0da0	0xffff0000000e0b40
0xffff0000000e0d90 <bcache+72568>:	0xffff000000118000	0xffff000000118000
0xffff0000000e0da0 <bcache+72584>:	0xffff0000000cf440	0xffff0000000e0d80
(gdb) x/16gx f
0xffff0000000e0db0 <freelist>:	0x9e670000b340bca3	0x0000000000000000
0xffff0000000e0dc0 <freelist+16>:	0x0000000000000000	0x0000000000000001
0xffff0000000e0dd0 <totalram>:	0x000000003bedb000	0xffff00000007ff50
0xffff0000000e0de0 <cpu+8>:	0xffff0000000e2cf8	0xffff0000000e26d8
0xffff0000000e0df0 <cpu+24>:	0x0000000000000000	0xffff00000007ef50
0xffff0000000e0e00 <cpu+40>:	0xffff0000000e20b8	0xffff0000000e20b8
0xffff0000000e0e10 <cpu+56>:	0x0000000000000000	0xffff00000007df50
0xffff0000000e0e20 <cpu+72>:	0xffff0000000e1a98	0xffff0000000e1a98
(gdb) q
```

## freelist

- freelistはfreelist構造体の変数
  - nextは最後のpageを指している
  - start, endはそれぞれ開始ページと最終ページを指している
- nextを*(struct freelist *)nextをしてでリファレンスすると前ページとなる
- kalloc()は最終ページを返す

  ```
  kalloc()
    return freelist_alloc(&freelist);
    freelist_alloc(struct freelist *f) {
      void *p = f->next;                    // freelist.next: 最終ページ
      if (p) f->next = *(void **)p;         // freelist.next: 最終ページの前ページ(最終ページをデリファレンスすると全ページとなる)
      return p;
    }
  ```

- kfree()は返されたページを最終ページとして挿入する

  ```
  kfree(void *va)
    freelist_free(&freelist, va);
    freelist_free(struct freelist *f, void *v) {
      *(void **)v = f->next;                // 現最終ページを返されたページの前ページとする
      f->next = v;                          // 返されたページを最終ページとする
    }
  ```

```
(gdb) p/x freelist
$16 = {
  next = 0xffff00003bfff000,
  start = 0xffff000000125000,
  end = 0xffff00003c000000
}
(gdb) p/x &freelist
$18 = 0xffff0000000e0db0
(gdb) p/x (struct freelist *)freelist
$20 = 0xffff00003bfff000
(gdb) p/x *(struct freelist *)freelist
$15 = {
  next = 0xffff00003bffe000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff00003bfff000
$17 = {
  next = 0xffff00003bffe000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff00003bffe000
$21 = {
  next = 0xffff00003bffd000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff00003bffd000
$22 = {
  next = 0xffff00003bffc000,
  start = 0x0,
  end = 0x0
}

(gdb) p/x freelist.start
$26 = 0xffff000000125000
(gdb) p/x *(struct freelist *)freelist.start
$6 = {
  next = 0x0,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)(freelist.start + 0x1000)
$7 = {
  next = 0xffff000000125000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)(freelist.start + 0x2000)
$8 = {
  next = 0xffff000000126000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff000000127000
$25 = {
  next = 0xffff000000126000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff000000126000
$24 = {
  next = 0xffff000000125000,
  start = 0x0,
  end = 0x0
}
(gdb) p/x *(struct freelist *)0xffff000000125000
$23 = {
  next = 0x0,
  start = 0x0,
  end = 0x0
}
```

## 結局これもvm_freeの異常削除でページ情報が壊れたためだと思われる

- [dashを導入](dash.md)に書いた変更でこの問題もなくなった
- 作業記録として残しておく
