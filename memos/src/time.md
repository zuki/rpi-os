# 07: 時計機能を追加

## `armstub8.S`による設定

- LOCAL_CONTROL (0x4000_000): 0
    * Bit 9 clear: Increment by 1 (vs. 2).
	* Bit 8 clear: Timer source is 19.2MHz crystal (vs. APB).
- LOCAL_PRESCALER (0x4000_0008): 0x8000_0000
    * 分周比 = (2^31 = 0x8000_0000 /0x8000_0000) = 1
    * タイマー周波数 = 分周比 * 入力周波数 = 19.2MHz
- CNTFRQ_EL0: 19200000

## 現状

- clock_intr()は1秒ごとに発生: ローカルタイマーを使用
- timer_intr()は各コアで1秒毎に発生: コアタイマーを使用
- 毎秒クロック割り込みが1回、コアタイマー割り込みが4回（各コアで1回）発生している
- クロック割り込みはTIMER_RELOAD_SEC、コアタイマー割り込みはdtの値を変えれば変更できる
  - ただし、コアタイマー割り込みをコア個別に変更することはできない

```
[0]clock_intr: c: 1
[1]timer_intr: t: 1
[0]timer_intr: t: 2
[2]timer_intr: t: 3
[3]timer_intr: t: 4

[0]clock_intr: c: 2
[1]timer_intr: t: 5
[0]timer_intr: t: 6
[2]timer_intr: t: 7
[3]timer_intr: t: 8
```

## プリエンプション

- 現状、timer_intr()で各CPU毎に1秒毎にプリエンプション
- 各cpuのidleプロセスがsys_yield()呼び出しになっている（無駄?）

## 時計関係の変数

- jiffies (uint64_t): システム時計で電源オンからのticksを保持
    - クロック割り込み(TIMER_RELOAD_SEC)で1増分
- xtime (struct timespec): 内部時計でepocを保持
    - 初期化時にRTSから設定し、以後はクロック割り込み時に増分

- 1 tickはHZ=100とすると10 millsec = 10K microsec = 10M nanosec

  ```
  // include/linux/jifies.h (2.6.15)による
  HZ = USER_HZ     =         100
  CCLOCK_TICK_RATE =  38,400,000
  LATCH            =     384,000
  ACTHZ            =      25,600      // HZは要求値。ACTHZは実際のHZ（"<< 8 "は精度のため）
  TICK_NSEC        =  10,000,000      // TICK_NSECは実ACTHZを想定したtick間時間（nsec）
  TICK_USEC        =      10,000      // TICK_USECは偽USER_HZを想定したtick間時間（usec）
  ```

# 方針

- 時計はclock()で行う
- プリエンプションはtimer()で行う

## 以下のファイルを編集または追加

```
inc/time.h              : 統計関係の構造体定義など
inc/i2c.h               : I2C関係のレジスタ定義、関数定義など
inc/ds3231.h            : RTC DS3231関係のレジスタ定義、関数定義など
inc/rtc.h               : RTC時間のget/set関数の定義
inc/clock.h             : cloc_gettime/settime関数の定義を追加
inc/types.h             : clockid_t型の定義

kern/i2c.c
kern/ds3231.c
kern/rtc.c
kern/clock.c
kern/timer.c
kern/syscall.c          : sys_clock_gettime/settimeを追加

usr/src/date/main.c     : 検証用dateコマンドの実装
usr/src/init/main.c     : TZ環境変数を設定
```


```
$ date
2022年 6日21日 火曜日 10時31分34秒 JST    // QEMUにはrtcがないので起動時の時間は固定
$ date
2022年 6日21日 火曜日 10時31分55秒 JST
```
