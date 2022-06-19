# 06: シグナル機能を追加

次のシステムコールを`sysproc.c`に実装

- sys_kill
- sys_rt_sigsuspend
- sys_rt_sigaction
- sys_rt_sigprocmask
- sys_rt_sigpending
- sys_rt_sigreturn

上のシステムコールの処理を担当する関数を`proc.c`に実装

- kill
- sigsuspend
- sigaction
- sigpending
- sigprocmask
- sigreturn
- handle_signal
- check_pending_signal
- flush_signal_handlers
- term_handler
- cont_handler
- stop_handler
- user_handler
- static send_signal

sys_returnを呼び出す関数を作成

- sigret_syscall.S

sigtestを実行するために以下のシステムコールを実装

- sys_ppoll
- sys_getpid
- sys_nanosleep
- sys_wait4 (wstatusのみ追加対応)

## sigtest

- ユーザハンドラが実行されていない
- ppollの実装の問題と思われる

```
$ sigtest
PID 8 ready
PID 9 ready
PID 10 ready
PID 11 ready
PID 12 ready
[1]sleep: ''(12) sleep lk=0xffff00000009aaa8
[0]sys_kill: pid=12, sig=2
[0]send_signal: pid=12, sig=2, state=2, paused=1
[0]handle_signal: [12]: sig=18
[0]cont_handler: pid=12
[0]wakeup1: wake ''(12)
12 is dead
```

### sigtest2の修正でsigtestは成功

```
$ sigtest
PID 8 ready
PID 9 ready
PID 10 ready
PID 11 ready
PID 12 ready
PID 12 caught sig 2
PID 11 caught sig 2
PID 10 caught sig 2
PID 9 caught sig 2
12 is dead
PID 8 caught sig 2
8 is dead
9 is dead
10 is dead
11 is dead
```

## sigtest2

```
$ sigtest2
sigtest2: A=0x400300
sigtest2: B=0x400330
sigtest2: C=0x400360
sigtest2: D=0x4003a0
sigtest2: E=0x4003d0

[1]sys_kill: pid=7, sig=10
[1]sys_kill: pid=7, sig=10
pid 7 function A got 10
[1]sys_kill: pid=7, sig=10
parent is sending signal to child
[1]sys_kill: pid=8, sig=12
[1]sys_kill: pid=8, sig=12
[1]check_pending_signal: pid=7, sig=10
[1]handle_signal: [7]: sig=10, handler=0x400330
pid 7 function B got 10
[2]check_pending_signal: pid=8, sig=12
[2]handle_signal: [8]: sig=12, handler=0x0
1 sleep  init
2 runble idle
3 runble idle
4 runble idle
5 run    idle
6 sleep  sh fa: 1
7 run    sigtest2 fa: 6
8 run     fa: 7
QEMU: Terminated
```

### sigtest2でスタック不足か

```c
int ppid = getpid();
sigaction(SIGUSR1, &act_ign, 0);
kill(ppid, SIGUSR1);

sigaction(SIGUSR1, &act_a, 0);
kill(ppid, SIGUSR1);

sigaction(SIGUSR1, &act_b, &oldact);
oldact.sa_handler(999);
kill(ppid, SIGUSR1);

```

実行すると3回目のsigactionでact_bのhandlerが0x0となっていた。

```
$ sigtest2
sigtest2: A=0x400290
sigtest2: B=0x4002c0

1st sigaction: SIG_IGN
[0]sigaction: act->handler=0x1
[0]sigaction: sig=10 handler: act=0x1, p=0x1
1st kill: default
[0]handle_signal: sig 10 handler is SIG_IGN
2nd sigaction: act_a
[0]sigaction: act->handler=0x400290
[0]sigaction: sig=10 handler: act=0x400290, p=0x400290
2nd kill: act_a
[0]handle_signal: call user_handler: sig = 10
pid 7 function A got 10
[0]sigaction: act->handler=0x0  // 設定するactのhandlerが0x0
[0]sigaction: oldact=0x400290
[0]sigaction: sig=10 handler: act=0x0, p=0x0
3rd sigaction: act_b and oldactcall A
pid 7 function A got 999
3rd kill: act_b                 // handlerはSIG_DFLとなり
[0]exit: exit: pid 7, err 1     // B()は呼ばれない
```

#### sig_a, sig_b等をグローバル変数にしてスタックを使わないように変更

```
sigtest2: A=0x400230
sigtest2: B=0x400260

1st sigaction: SIG_IGN
[3]sigaction: act->handler=0x1
[3]sigaction: sig=10 handler: act=0x1, p=0x1
1st kill: default
[3]handle_signal: sig 10 handler is SIG_IGN
2nd sigaction: act_a
[3]sigaction: act->handler=0x400230
[3]sigaction: sig=10 handler: act=0x400230, p=0x400230
2nd kill: act_a
[3]handle_signal: call user_handler: sig = 10
pid 7 function A got 10
[3]sigaction: act->handler=0x400260     // 設定するactのhandlerがB()
[3]sigaction: oldact=0x400230
[3]sigaction: sig=10 handler: act=0x400260, p=0x400260
3rd sigaction: act_b and oldactcall A
pid 7 function A got 999
3rd kill: act_b
[3]handle_signal: call user_handler: sig = 10
pid 7 function B got 10         // B()が呼ばれた
```

### check_pending_signal()を呼び出すタイミングの問題

#### timer()によるpreemption時のみ呼び出す

- 2回目のkill()が無視されている

```
$ sigtest2
pid 7 function A got 999
parent is sending signal to child
pid 8 function B got 10
parent is sending signal to child
pid 7 function B got 10
pid 8 function C got 12
parent got signal from child
```

#### syscall()後にも呼び出す

- プロセス内は正しく処理されている
- プロセス間はまだおかしい

```
$ sigtest2
pid 7 function A got 10
pid 7 function A got 999
pid 7 function B got 10
parent is sending signal to child
pid 8 function C got 12
parent is sending signal to child
parent got signal from child
```

### プロセス間の

### ローカル変数版

- muslの`sigset_t`は128バイト
  - `typedef struct __sigset_t { unsigned long __bits[128/sizeof(long)]; } sigset_t;`

```
$ readelf -l sigtest2
Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000001000 0x0000000000400000 0x0000000000400000
                 0x0000000000007220 0x0000000000007220  R E    0x1000
  LOAD           0x0000000000008220 0x0000000000408220 0x0000000000408220
                 0x0000000000000138 0x00000000000007d0  RW     0x1000
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10

$ less obj/usr/src/sigtest2/sigtest2.asm
0000000000400030 <main>:
  400030:   d10c83ff    sub sp, sp, #0x320              // 0x320 = 800
  400034:   d2800022    mov x2, #0x1                    // x2 = SIG_IGN
  400038:   4f000400    movi    v0.4s, #0x0             // v0.4s: 32bit*4
  40003c:   910323e1    add x1, sp, #0xc8               // x1 = 0xc8 = 200
  400040:   9107e3e0    add x0, sp, #0x1f8              // x0 = 0x1f8 = 504
  400044:   90000004    adrp    x4, 400000 <_init>
  400048:   a9007bfd    stp x29, x30, [sp]
  40004c:   910003fd    mov x29, sp
  400050:   91084084    add x4, x4, #0x210              // X4 = func A
  400054:   90000003    adrp    x3, 400000 <_init>
  400058:   91090063    add x3, x3, #0x240              // X3 = func B
  40005c:   a90153f3    stp x19, x20, [sp, #16]
  400060:   f90063e2    str x2, [sp, #192]
  400064:   90000002    adrp    x2, 400000 <_init>
  400068:   9109c042    add x2, x2, #0x270              // x2 = func C
  40006c:   f900afe4    str x4, [sp, #344]
  400070:   f900fbe3    str x3, [sp, #496]
  400074:   ad000020    stp q0, q0, [x1]                // このq0はstruct sigactionのsa_flags, sa_mask, sa_restorerの0クリアをしている
  400078:   ad000000    stp q0, q0, [x0]                // 特にsa_maskは128バイトある
  40007c:   ad010020    stp q0, q0, [x1, #32]
  400080:   ad010000    stp q0, q0, [x0, #32]
  400084:   ad020020    stp q0, q0, [x1, #64]
  400088:   ad020000    stp q0, q0, [x0, #64]
  40008c:   ad030020    stp q0, q0, [x1, #96]
  400090:   ad0b03e0    stp q0, q0, [sp, #352]
  400094:   ad0c03e0    stp q0, q0, [sp, #384]
  400098:   ad0d03e0    stp q0, q0, [sp, #416]
  40009c:   ad0e03e0    stp q0, q0, [sp, #448]
  4000a0:   ad1483e0    stp q0, q0, [sp, #656]
  4000a4:   ad030000    stp q0, q0, [x0, #96]
  4000a8:   3d802020    str q0, [x1, #128]
  4000ac:   3d802000    str q0, [x0, #128]
  4000b0:   3d807be0    str q0, [sp, #480]
  4000b4:   f90147e2    str x2, [sp, #648]
  4000b8:   ad1583e0    stp q0, q0, [sp, #688]
  4000bc:   ad1683e0    stp q0, q0, [sp, #720]
  4000c0:   ad1783e0    stp q0, q0, [sp, #752]
  4000c4:   3d80c7e0    str q0, [sp, #784]
  4000c8:   94000adc    bl  402c38 <getpid>
  4000cc:   d2800002    mov x2, #0x0                    // #0
  4000d0:   2a0003f3    mov w19, w0                     // x19 = ppid
  4000d4:   910303e1    add x1, sp, #0xc0               // act_ign
  4000d8:   52800140    mov w0, #0xa                    // #10
  4000dc:   94000222    bl  400964 <__sigaction>
  4000e0:   52800141    mov w1, #0xa                    // SIGUSR1 #10
  4000e4:   2a1303e0    mov w0, w19                     // ppid
  4000e8:   940001b2    bl  4007b0 <kill>
  4000ec:   d2800002    mov x2, #0x0                    // #0
  4000f0:   910563e1    add x1, sp, #0x158              // sig_a
  4000f4:   52800140    mov w0, #0xa                    // #10
  4000f8:   9400021b    bl  400964 <__sigaction>
  4000fc:   52800141    mov w1, #0xa                    // #10
  400100:   2a1303e0    mov w0, w19
  400104:   940001ab    bl  4007b0 <kill>

```

### グローバル変数版

```
$ readelf -l sigtest2
Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000001000 0x0000000000400000 0x0000000000400000
                 0x00000000000071a0 0x00000000000071a0  R E    0x1000
  LOAD           0x00000000000081a0 0x00000000004081a0 0x00000000004081a0
                 0x00000000000004c8 0x0000000000000b60  RW     0x1000
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10

$ less obj/usr/src/sigtest2/sigtest2.asm
0000000000400030 <main>:
  400030:   a9b47bfd    stp x29, x30, [sp, #-192]!
  400034:   910003fd    mov x29, sp
  400038:   a90153f3    stp x19, x20, [sp, #16]
  40003c:   94000adf    bl  402bb8 <getpid>
  400040:   d2800002    mov x2, #0x0                    // #0
  400044:   90000053    adrp    x19, 408000 <__stdio_ofl_lockptr+0xe68>
  400048:   91076273    add x19, x19, #0x1d8
  40004c:   2a0003f4    mov w20, w0
  400050:   aa1303e1    mov x1, x19
  400054:   52800140    mov w0, #0xa                    // #10
  400058:   94000223    bl  4008e4 <__sigaction>
  40005c:   52800141    mov w1, #0xa                    // #10
  400060:   2a1403e0    mov w0, w20
  400064:   940001b3    bl  400730 <kill>
```

### 6/18現在の状況

```
$ sigtest2
pid 7 function A got 10
pid 7 function A got 999
pid 7 function B got 10
parent is sending signal to child: pid=7
pid 8 function C got 12
parent is sending signal to child: pid=8
parent got signal from child
pid 9 function D got 3                      // ここで止まる
```

## sigtest3

```
$ sigtest3
[1]sys_rt_sigprocmask: how=0, act=0x407dd0, oldact=0xfffffffffea0, size=8
[1]sys_rt_sigprocmask: how=0, act=0x407dc8, oldact=0xfffffffffde0, size=8
[1]sys_rt_sigprocmask: how=2, act=0xfffffffffde0, oldact=0x0, size=8
[2]sys_rt_sigprocmask: how=2, act=0xfffffffffde0, oldact=0x0, size=8
[1]sys_rt_sigprocmask: how=2, act=0xfffffffffea0, oldact=0x0, size=8
[2]sys_rt_sigprocmask: how=2, act=0xfffffffffea0, oldact=0x0, size=8
[2]sys_rt_sigprocmask: how=1, act=0xfffffffffce0, oldact=0x0, size=8
mask1: 0xfffffd50
mask2: 0xfffffd50
[3]sys_rt_sigprocmask: how=0, act=0xfffffffffdd8, oldact=0xfffffffffe58, size=8
org_mask: 0xfffffd50
[0]sys_rt_sigprocmask: how=2, act=0xfffffffffe58, oldact=0x0, size=8
Wrong done
```

- テストコードが間違っていた

```
$ sigtest3
Got signal!
```

## 修正

- check_pending_signal()の呼び出しをsyscall1(), irq_handler()実行後とした
- struct k_sigactionのmaskフィールドの定義を変更した

### sigtest, sigtest3は成功, sigtest2は失敗

- sigtest2はコード自体、要件等か?
- とりあえずsignal機能の実行はこれで良しとする

```
$ sigtest
PID 8 ready
PID 9 ready
PID 10 ready
PID 11 ready
PID 12 ready
PID 12 caught sig 2
PID 11 caught sig 2
PID 10 caught sig 2
PID 9 caught sig 2
12 is dead
PID 8 caught sig 2
9 is dead
8 is dead
10 is dead
11 is dead
$ sigtest3
Got signal!
$ sigtest2
pid 14 function A got 10
pid 14 function A got 999
pid 14 function B got 10
parent is sending signal to child: pid=14
pid 15 function C got 12
parent is sending signal to child: pid=15
parent got signal from child
pid 16 function D got 3
QEMU: Terminated
```

### sigtest2のfork間でのsignalのやり取り部分のコードを修正

- forkするとSIG_IGN以外はSIG_DFLにクリアされるので子プロセス側でハンドラの再設定が必要だった

```
$ sigtest2
part1 start
PID 7 function A got 10
PID 7 function A got 999
PID 7 function B got 10

part2 start
PID 7 sends signal to PID 8
PID 8 function C got 10
PID 8 got signal and sends signal to PID 7
PID 7 function C got 10
PID 7 got signal from PID 8
$
```

- part3でDとEを1ずつ設定し、処理の間に適当にsleepを入れる

```
$ sigtest2
part1 start
PID 7 function A got 10
PID 7 function A got 999
PID 7 function B got 10

part2 start
PID 7 sends signal to PID 8
PID 8 function C got 10
PID 8 got signal and sends signal to PID 7
PID 7 function C got 10
PID 7 got signal from PID 8

part3 start
PID 9 function D got 10
PID 9 function E got 12
bye bye

all ok
```
