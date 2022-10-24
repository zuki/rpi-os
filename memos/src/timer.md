# timer機能を実装

## 実行はするがストールする

```
# timertest 2
       Elapsed   Value Interval
START:    0.01                                  // 実行はするが時間が来ても反応なし
[Cntl-P]
Total Memory: 945 MB, Free Memmory: 940 MB

1 sleep  init
2 runble idle
3 runble idle
4 run    idle
5 run    idle
6 sleep  dash fa: 1
7 run    itimertest fa: 6
```

### `init_timervecs()`をしていないためだった


```
# date
Tue Jun 21 10:31:31 JST 2022
# timertest
       Elapsed   Value Interval
START:    0.01
Main:     2.02    0.00    0.00
ALARM:    2.02    0.00    0.00
That's all folks
# date
Tue Jun 21 10:31:57 JST 2022
# timertest 2 0 1 0
       Elapsed   Value Interval
START:    0.01
Main:     2.02    1.00    1.00
ALARM:    2.02    1.00    1.00
Main:     3.02    1.00    1.00
ALARM:    3.02    1.00    1.00
Main:     4.02    1.00    1.00
ALARM:    4.02    1.00    1.00
That's all folks
#
```

## jiffiesの初期値で処理が変わる

- 初期値を負値にするのは意味があったはずだが、当面ゼロにする
- 最初にdateコマンドを実行すると以下はclock割り込みが行われるようだ

| define                                 |             初期値 | 動作                |
| :------------------------------------- | -----------------: | :------------------ |
| ((uint64_t)-300 * HZ)                  | 0xFFFFFFFFFFFF8AD0 | itimerが動かず      |
| (((uint64_t)(-300 * HZ)) & 0xFFFFFFFF) | 0x00000000FFFF8AD0 | clock割り込みしない |
| 0UL                                    | 0x0000000000000000 | 正常稼働            |


## 変更履歴

```
$ git status
On branch mac
Changes to be committed:
	modified:   README.md
	modified:   inc/linux/time.h
	modified:   inc/list.h
	modified:   inc/proc.h
	modified:   inc/syscall1.h
	modified:   kern/clock.c
	new file:   kern/itimer.c
	modified:   kern/main.c
	modified:   kern/proc.c
	modified:   kern/syscall.c
	modified:   kern/timer.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/timer.md
	new file:   usr/src/timertest/main.c
```

## 問題を認識

### Macで実行した場合の結果

```
$ ./timer 2 0 1 0
       Elapsed   Value Interval
START:    0.00
Main:     0.50    1.50    1.00
Main:     1.00    1.00    1.00
Main:     1.51    0.49    1.00
ALARM:    2.01    0.99    1.00
Main:     2.01    0.99    1.00
Main:     2.51    0.49    1.00
ALARM:    3.00    1.00    1.00
Main:     3.01    0.99    1.00
Main:     3.51    0.49    1.00
ALARM:    4.00    1.00    1.00
That's all folks
```

### 相違点

- Macでは0.5秒ごとに表示されるMain行でValue値にexpireするまでの残り時間が設定されている
- XV6ではALARM時にしかMain行が表示されない
  - これについては`clock()`コマンドが常に`-1`を返しているため
  - それだとMain行は一度も出力されないはず
- clock()が-1を返すのはmsulでの実装が`__syscall(SYS_clock_gettime, CLOCK_PROCESS_CPUTIME_ID, &ts)`であり、sys_clock_gettimeはCLOCK_PROCESS_CPUTIME_IDに対応していないので`-EINVAL`を返しているため
- CLOCK_PROCESS_CPUTIME_IDの場合とりあえずCLOCK_REALTIMEとして処理するようにするとMain行の振る舞いは正しくなるががALARMが発行されない

```
# timertest
       Elapsed   Value Interval
START:    0.01
Main:     0.53     inf     nan
Main:     1.03    1.00    0.00
Main:     1.53    0.50    0.00
Main:     2.03    0.00    0.00			// ここでストール
QEMU: Terminated
```

## procに現在時を持ち、nano秒単位のjiffiesを設定

```diff
$ git diff inc/proc.h
diff --git a/inc/proc.h b/inc/proc.h
index 0717d68..1abb66a 100644
--- a/inc/proc.h
+++ b/inc/proc.h
@@ -87,6 +87,7 @@ struct proc {
     kernel_cap_t   cap_effective, cap_inheritable, cap_permitted;   // capabilities
     mode_t umask;               // umask

+    uint64_t stime, utime;      // ticks for system and user
     uint64_t it_real_value, it_prof_value, it_virt_value;   /* timer interval value: REAL, PROF, VIRTUAL */
     uint64_t it_real_incr, it_prof_incr, it_virt_incr;      /* timer increment: REAL, PROF, VIRTUAL */
     struct timer_list real_timer;                           /* Real time timer */
$ git diff kern/clock.c
diff --git a/kern/clock.c b/kern/clock.c
index 589f629..78bc40b 100644
--- a/kern/clock.c
+++ b/kern/clock.c
@@ -1,10 +1,12 @@
 #include "clock.h"
+#include "linux/errno.h"
 #include "linux/time.h"
 #include "arm.h"
 #include "base.h"
 #include "irq.h"
 #include "console.h"
 #include "rtc.h"
+#include "proc.h"

 /* Local timer */
 #define LOCAL_TIMER_ROUTE             (LOCAL_BASE + 0x24)
@@ -92,6 +94,7 @@ void
 clock_intr()
 {
     ++jiffies;
+    thisproc()->stime = jiffies * 1000000000 / HZ;
     trace("c: %d", jiffies);
     update_times();
     run_timer_list();
@@ -101,20 +104,39 @@ clock_intr()
 long
 clock_gettime(clockid_t clk_id, struct timespec *tp)
 {
-    // TODO: clk_idによる振り分け
-    tp->tv_nsec = xtime.tv_nsec;
-    tp->tv_sec = xtime.tv_sec;
-
-    trace("tv_sec: %lld, tv_nsec: %lld", tp->tv_sec, tp->tv_nsec);
+    uint64_t ptime;
+
+    switch(clk_id) {
+        default:
+            return -EINVAL;
+        case CLOCK_REALTIME:
+            tp->tv_nsec = xtime.tv_nsec;
+            tp->tv_sec = xtime.tv_sec;
+            break;
+        case CLOCK_PROCESS_CPUTIME_ID:
+            ptime = thisproc()->stime + thisproc()->utime;
+            tp->tv_nsec = ptime % 1000000000;
+            tp->tv_sec  = ptime / 1000000000;
+            break;
+    }
+    debug("clk: %d, tv_sec: %lld, tv_nsec: %lld", clk_id, tp->tv_sec, tp->tv_nsec);
     return 0;
 }

 long clock_settime(clockid_t clk_id, const struct timespec *tp)
 {
-    // TODO: 権限チェック, lock?
-    xtime.tv_nsec = tp->tv_nsec;
-    xtime.tv_sec = tp->tv_sec;
-
+    switch(clk_id) {
+        default:
+            return -EINVAL;
+        case CLOCK_REALTIME:
+            if (capable(CAP_SYS_TIME)) {
+                xtime.tv_nsec = tp->tv_nsec;
+                xtime.tv_sec = tp->tv_sec;
+            } else {
+                return -EPERM;
+            }
+            break;
+    }
     return 0;
 }
```

- かなりずれるが正式な計時はTODO

```
# date
Tue Jun 21 10:31:26 JST 2022
# timertest
       Elapsed   Value Interval
START:    0.00
Main:     0.49    1.53    0.00
Main:     1.04    0.98    0.00
Main:     1.62    0.40    0.00
ALARM:    2.02    0.00    0.00
That's all folks
# timertest 2 0 1 0
       Elapsed   Value Interval
START:    0.00
Main:     0.51    1.50    1.00
Main:     1.15    0.86    1.00
Main:     1.71    0.30    1.00
ALARM:    2.01    1.00    1.00
Main:     2.24    0.77    1.00
Main:     2.77    0.24    1.00
ALARM:    3.01    1.00    1.00
Main:     3.45    0.56    1.00
Main:     3.96    0.05    1.00
ALARM:    4.01    1.00    1.00
That's all folks
# date
Tue Jun 21 10:32:25 JST 2022
```
