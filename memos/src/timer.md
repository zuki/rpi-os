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
