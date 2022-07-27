# ダイナミックリンク機能を追加

- `~/musl/lib/musl-gcc.specs`の`-dynamic-linker`を`/lib/ld-musl-aarch64.so.1`に変更
- ~`/musl/lib/libc.so`を`usr/lib/libc.so`にコピーし
- inittabで`/lib/libc.so`を`/lib/ld-musl-aarch64.so.1`にシンボリックリンク

```
$ hello-dyn
-dash: 3: hello-dyn: Operation not permitted
```

## 問題1: load_interpreter()が-EINVAL

- get_file(path)のip->type != T_FILEが該当
- シンボリックリンクをたどる機能が未実装だった

### 問題1 解決

- シンボリックリンクをたどる機能を実装

## 問題2: execve()は実効されたがその後ストール

```
$ hello-dyn
[0]execve: [11] parse /bin/hello-dyn
[0]execve: argv[0]: hello-dyn, envp[0]: PWD=/home/zuki, envp[1]: TZ=JST-9
[0]execve: sp1: 0x1000000000000
[0]execve: sp2: 0xfffffffffff6
[0]execve: sp2: 0xffffffffffe7
[0]execve: sp2: 0xffffffffffde
[0]execve: sp3: 0xffffffffffd6
[0]execve: argc: 1, envc: 2, auxc: 15, argv: 0xfffffffffea8, envp: 0xfffffffffeb8, auxv: 0xfffffffffed0
[0]execve: sp4: 0xffffffffffd6
[0]execve: newenvp[1]: 0xfffffffffec0 = aarch64             // ずれている
[0]execve: newenvp[0]: 0xfffffffffeb8 = TZ=JST-9            // ずれている
[0]execve: sp5: 0xffffffffffe7
[0]execve: newargv[0]: 0xfffffffffea8 = PWD=/home/zuki      // ずれている
[0]execve: sp6: 0xfffffffffff6
[0]execve: sp7: 0xfffffffffea0
[0]execve: stksz: 0xa000, sp: 0xfffffffffea0
[0]execve: copyout: 0xffffffff6000, 0, 0x9ea0
fffffffffff0: 656800696b757a2f 006e79642d6f6c6c /zuki.hello-dyn.
ffffffffffe0: 5000392d54534a3d 656d6f682f3d4457 =JST-9.PWD=/home
ffffffffffd0: 6161000000000000 5a54003436686372 ......aarch64.TZ
ffffffffffc0: 0000000000000000 0000000000000000 ................
ffffffffffb0: 0000000000000000 0000000000000000 ................
ffffffffffa0: 0000000000000011 0000000000000064 ........d.......
ffffffffff90: 0000000000000010 0000000000000887 ................
ffffffffff80: 000000000000000f 0000ffffffffffd6 ................
ffffffffff70: 000000000000000e 00000000000003e8 ................
ffffffffff60: 000000000000000d 00000000000003e8 ................
ffffffffff50: 000000000000000c 00000000000003e8 ................
ffffffffff40: 000000000000000b 00000000000003e8 ................
ffffffffff30: 0000000000000009 00000000004001f0 ..........@.....
ffffffffff20: 0000000000000008 0000000000000000 ................
ffffffffff10: 0000000000000007 0000c00000000000 ................
ffffffffff00: 0000000000000006 0000000000001000 ................
fffffffffef0: 0000000000000005 0000000000000006 ................
fffffffffee0: 0000000000000004 0000000000000038 ........8.......
fffffffffed0: 0000000000000003 0000000000000040 ........@.......
fffffffffec0: 0000ffffffffffd6 0000000000000000 ................
fffffffffeb0: 0000000000000000 0000ffffffffffde ................
fffffffffea0: 0000000000000001 0000ffffffffffe7 ................
```

## 問題2 解決

- auxv, envp, argvを設定する処理で、auxvで使用するplatform文字列のの設定位置の問題で設定がずれていた
- platformを最初に設定するよう変更した

```
$ hello-dyn
[2]execve: [11] parse /bin/hello-dyn
[2]execve: argv[0]: hello-dyn, envp[0]: PWD=/home/zuki, envp[1]: TZ=JST-9
[2]execve: sp1: 0x1000000000000
[2]execve: sp2: 0xfffffffffff8
[2]execve: sp3: 0xffffffffffee
[2]execve: sp4: 0xffffffffffdf
[2]execve: sp4: 0xffffffffffd6
[2]execve: argc: 1, envc: 2, auxc: 15, argv: 0xfffffffffea8, envp: 0xfffffffffeb8, auxv: 0xfffffffffed0
[2]execve: sp5: 0xffffffffffd6
[2]execve: newenvp[1]: 0xfffffffffec0 = TZ=JST-9                    // 正しい
[2]execve: newenvp[0]: 0xfffffffffeb8 = PWD=/home/zuki              // 正しい
[2]execve: sp6: 0xffffffffffee
[2]execve: newargv[0]: 0xfffffffffea8 = hello-dyn                   // 正しい
[2]execve: sp7: 0xfffffffffff8
[2]execve: sp8: 0xfffffffffea0
[2]execve: stksz: 0xa000, sp: 0xfffffffffea0
[2]execve: copyout: 0xffffffff6000, 0, 0x9ea0
fffffffffff0: 006e79642d6f6c6c 0034366863726161 llo-dyn.aarch64.
ffffffffffe0: 656d6f682f3d4457 656800696b757a2f WD=/home/zuki.he
ffffffffffd0: 5a54000000000000 5000392d54534a3d ......TZ=JST-9.P
ffffffffffc0: 0000000000000000 0000000000000000 ................
ffffffffffb0: 0000000000000000 0000000000000000 ................
ffffffffffa0: 0000000000000011 0000000000000064 ........d.......
ffffffffff90: 0000000000000010 0000000000000887 ................
ffffffffff80: 000000000000000f 0000fffffffffff8 ................
ffffffffff70: 000000000000000e 00000000000003e8 ................
ffffffffff60: 000000000000000d 00000000000003e8 ................
ffffffffff50: 000000000000000c 00000000000003e8 ................
ffffffffff40: 000000000000000b 00000000000003e8 ................
ffffffffff30: 0000000000000009 00000000004001f0 ..........@.....
ffffffffff20: 0000000000000008 0000000000000000 ................
ffffffffff10: 0000000000000007 0000c00000000000 ................
ffffffffff00: 0000000000000006 0000000000001000 ................
fffffffffef0: 0000000000000005 0000000000000006 ................
fffffffffee0: 0000000000000004 0000000000000038 ........8.......
fffffffffed0: 0000000000000003 0000000000000040 ........@.......
fffffffffec0: 0000ffffffffffd6 0000000000000000 ................
fffffffffeb0: 0000000000000000 0000ffffffffffdf ................
fffffffffea0: 0000000000000001 0000ffffffffffee ................
```

## 問題3 依然としてストール

- xv6-armv8のdynamicブランチと比較したところ、Elf種別が違っていた

### xv6-armv8

```
$ file obj/dyn/bin/hello-dyn
obj/dyn/bin/hello-dyn: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped

$ aarch64-elf-readelf -hlW obj/dyn/bin/hello-dyn
  Type:                              EXEC (Executable file)
  Machine:                           AArch64
  Entry point address:               0x4001f0
```

### rpi-os

```
$ file obj/user_dyn/bin/hello
obj/user_dyn/bin/hello: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped
$ aarch64-linux-gnu-readelf -h obj/user_dyn/bin/hello
  Type:                              DYN (Position-Independent Executable file)
  Machine:                           AArch64
  Version:                           0x1
  Entry point address:               0x560
  Start of program headers:          64 (bytes into file)
  Start of section headers:          6568 (bytes into file)
```

### 問題3 解決

- aarch-elf-gccではコンパイルフラグとリンクフラグで`-fpie`と`pie`を指定しないとpie実行形式にならなかった

```
$ ~/musl/bin/musl-gcc -o hello-musl hello.c
$ file hello-musl
hello-musl: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped

$ ~/musl/bin/musl-gcc -fpie -pie -o hello-pie hello.c
$ file hello-pie
hello-pie: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-musl-aarch64.so.1, not stripped
```

- dyn/Makefileに両フラグを追加

```
# hello-dyn
Hello dynamic world!            // 動いた
#
```


## vm_stat

```
$ hello-dyn
[0]vm_stat: va: 0x3ff000, pa: 0xffff00003ad21000, pte: 0x3ad21747, PTE_ADDR(pte): 0x3ad21000, P2V(...): 0xffff00003ad21000
[0]vm_stat: va: 0x400000, pa: 0xffff00003ad1d000, pte: 0x3ad1d747, PTE_ADDR(pte): 0x3ad1d000, P2V(...): 0xffff00003ad1d000
[0]vm_stat: va: 0x401000, pa: 0xffff00003ad1b000, pte: 0x3ad1b747, PTE_ADDR(pte): 0x3ad1b000, P2V(...): 0xffff00003ad1b000
[0]vm_stat: va: [0x3ff000 ~ 0x402000)
[0]vm_stat: va: 0xc00000000000, pa: 0xffff00003ad1a000, pte: 0x3ad1a747, PTE_ADDR(pte): 0x3ad1a000, P2V(...): 0xffff00003ad1a000
...
[0]vm_stat: va: 0xc00000098000, pa: 0xffff00003ac7f000, pte: 0x3ac7f747, PTE_ADDR(pte): 0x3ac7f000, P2V(...): 0xffff00003ac7f000
[0]vm_stat: va: [0xc00000000000 ~ 0xc00000099000)
[0]vm_stat: va: 0xc000000a8000, pa: 0xffff00003ac7e000, pte: 0x3ac7e747, PTE_ADDR(pte): 0x3ac7e000, P2V(...): 0xffff00003ac7e000
...
[0]vm_stat: va: 0xc000000ab000, pa: 0xffff00003ac7b000, pte: 0x3ac7b747, PTE_ADDR(pte): 0x3ac7b000, P2V(...): 0xffff00003ac7b000
[0]vm_stat: va: [0xc000000a8000 ~ 0xc000000ac000)
[0]vm_stat: va: 0xffffffff6000, pa: 0xffff00003ac76000, pte: 0x3ac76747, PTE_ADDR(pte): 0x3ac76000, P2V(...): 0xffff00003ac76000
...
[0]vm_stat: va: 0xfffffffff000, pa: 0xffff00003ac77000, pte: 0x3ac77747, PTE_ADDR(pte): 0x3ac77000, P2V(...): 0xffff00003ac77000
[0]vm_stat: va: [0xffffffff6000 ~ 0x1000000000000)
```
