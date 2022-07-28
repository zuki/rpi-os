# ファイル読み込みにpagecacheを使う

- execve()のreadiの代わりにcopy_oage()を使う
- copy_page()はページ単位のため、複数ページを一度にコピーするcopy_pages()を作成
- インタプリタはmmapしているのでもともとcopy_page()が使われていた
- 本来はプログラム本体もmmapするべきかもしれないがしていない

```
[1]execve: [6] parse /usr/bin/dash
[1]copy_page: inum=146, offset=0x0, dest=0xffff000038bfccf0, size=0x40, dest_offset=0x0
[1]get_page: alloc new cached page[4]: ip=146, ffset=0x0                                // 初回はpage読み込み
[1]copy_page: inum=146, offset=0x0, dest=0xffff000038bdfe90, size=0xa8, dest_offset=0x40
[1]find_page: found page [4]                                                            // page cacheにヒット
[1]execve: vaddr: 0x400000, filesz: 0x2b4b4, offset: 0x1000
[1]copy_pages: pages: 44
[1]copy_page: inum=146, offset=0x1000, dest=0x400000, size=0x1000, dest_offset=0x0
[1]get_page: alloc new cached page[5]: ip=146, offset=0x1000                            // 初回はpage読み込み
[1]copy_page: inum=146, offset=0x2000, dest=0x401000, size=0x1000, dest_offset=0x0
[1]get_page: alloc new cached page[6]: ip=146, offset=0x2000
[1]copy_page: inum=146, offset=0x3000, dest=0x402000, size=0x1000, dest_offset=0x0
[1]get_page: alloc new cached page[7]: ip=146, offset=0x3000
...

Password:
[2]execve: [10] parse /usr/bin/dash
[2]copy_page: inum=146, offset=0x0, dest=0xffff000038a50cf0, size=0x40, dest_offset=0x0
[2]find_page: found page [4]
[2]copy_page: inum=146, offset=0x0, dest=0xffff000038bdf850, size=0xa8, dest_offset=0x40
[2]find_page: found page [4]
[2]execve: vaddr: 0x400000, filesz: 0x2b4b4, offset: 0x1000
[2]copy_pages: pages: 44
[2]copy_page: inum=146, offset=0x1000, dest=0x400000, size=0x1000, dest_offset=0x0
[2]find_page: found page [5]                                                            // page cacheにヒット
[2]copy_page: inum=146, offset=0x2000, dest=0x401000, size=0x1000, dest_offset=0x0
[2]find_page: found page [6]
[2]copy_page: inum=146, offset=0x3000, dest=0x402000, size=0x1000, dest_offset=0x0
[2]find_page: found page [7]
[2]copy_page: inum=146, offset=0x4000, dest=0x403000, size=0x1000, dest_offset=0x0
[2]find_page: found page [8]
...

$ readelf -h /bin/ls
[0]execve: [11] parse /usr/bin/readelf
[0]copy_page: inum=153, offset=0x0, dest=0xffff0000389dbcf0, size=0x40, dest_offset=0x0
[0]get_page: alloc new cached page[158]: ip=153, offset=0x0
[0]copy_page: inum=153, offset=0x0, dest=0xffff000038bdf630, size=0x150, dest_offset=0x40
[0]find_page: found page [158]
[0]execve: unsupported type 0x6, skipped

[0]copy_page: inum=153, offset=0x0, dest=0xffff000038bdf600, size=0x1a, dest_offset=0x190
[0]find_page: found page [158]
[0]execve: vaddr: 0x0, filesz: 0xf1c50, offset: 0x0
[0]copy_pages: pages: 242
[0]copy_page: inum=153, offset=0x0, dest=0x0, size=0x1000, dest_offset=0x0
[0]find_page: found page [158]
[0]copy_page: inum=153, offset=0x1000, dest=0x1000, size=0x1000, dest_offset=0x0
[0]get_page: alloc new cached page[159]: ip=153, offset=0x1000
[0]copy_page: inum=153, offset=0x2000, dest=0x2000, size=0x1000, dest_offset=0x0
[0]get_page: alloc new cached page[160]: ip=153, offset=0x2000
...
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           AArch64
  Version:                           0x1
  Entry point address:               0x400088
  Start of program headers:          64 (bytes into file)
  Start of section headers:          52232 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         3
  Size of section headers:           64 (bytes)
  Number of section headers:         13
  Section header string table index: 12

$ readelf -lW /bin/ls
[0]execve: [12] parse /usr/bin/readelf
[0]copy_page: inum=153, offset=0x0, dest=0xffff000038922cf0, size=0x40, dest_offset=0x0
[0]find_page: found page [158]
[0]copy_page: inum=153, offset=0x0, dest=0xffff000038bdf270, size=0x150, dest_offset=0x40
[0]find_page: found page [158]
[0]execve: unsupported type 0x6, skipped

[0]copy_page: inum=153, offset=0x0, dest=0xffff000038bdf240, size=0x1a, dest_offset=0x190
[0]find_page: found page [158]
[0]execve: vaddr: 0x0, filesz: 0xf1c50, offset: 0x0
[0]copy_pages: pages: 242
[0]copy_page: inum=153, offset=0x0, dest=0x0, size=0x1000, dest_offset=0x0
[0]find_page: found page [158]
[0]copy_page: inum=153, offset=0x1000, dest=0x1000, size=0x1000, dest_offset=0x0
[0]find_page: found page [159]
[0]copy_page: inum=153, offset=0x2000, dest=0x2000, size=0x1000, dest_offset=0x0
[0]find_page: found page [160]
...

Elf file type is EXEC (Executable file)
Entry point 0x400088
There are 3 program headers, starting at offset 64

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  LOAD           0x001000 0x0000000000400000 0x0000000000400000 0x0088e8 0x0088e8 R E 0x1000
  LOAD           0x0098e8 0x00000000004098e8 0x00000000004098e8 0x000230 0x0009f0 RW  0x1000
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10

 Section to Segment mapping:
  Segment Sections...
   00     .init .text .fini .rodata
   01     .got .got.plt .data .bss
   02
$
```

## 更新履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	modified:   README.md
	modified:   inc/linux/elf.h
	modified:   inc/pagecache.h
	modified:   kern/exec.c
	modified:   kern/pagecache.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/pagecache.md
```
