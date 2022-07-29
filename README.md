# Raspberry Piオペレーティングシステム

Raspberry Pi 3/4で稼働するunixライクのもう一つのトイOSです。復旦大学のオペレーティングシステムコースの[研究用](https://github.com/FDUCSLG/OS-2020Fall-Fudan/) に古典的な[xv6](https://github.com/mit-pdos/xv6-public/)のフレームワークに基づいて作成されました。

Raspberry Pi 3A+, 3B+, 4Bでテストしています。

## 関連プロジェクト

- [linux](https://github.com/raspberrypi/linux): 現実世界のオペレーティングシステム
- [circle](https://github.com/rsta2/circle): 多くのポータブルなドライバを含んでいます。
- [s-matyukevichのraspberry-pi-os](https://github.com/s-matyukevich/raspberry-pi-os)
- [bztsrcのraspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial)

## XV6との違い

- ユーザプログラム用のlbcは再発明せずに[musl](https://musl.libc.org/)を使用しています。
- カーネルがサポートしているシステムコールはLinuxのシステムコールのサブセットです。
- xv6とは違い、プロセスの待機と起床（sleepingとwaking）にキューを使用したスケジューラとハッシュpidを使用しています。

## 機能

- [x] AArch64だけに対応
- [x] 基本的なマルチコアのサポート
- [x] メモリ管理
- [x] スワップ機能を持たない仮想メモリ
- [x] プロセス管理
- [x] ディスクドライバ（EMMC）: [circle](https://github.com/rsta2/circle/tree/master/addon/SDCard)からポーティング
- [x] ファイルシステム: xv6からポーティング
- [x] Cライブラリ: [musl](https://musl.libc.org/)
- [x] シェル: xv6からポーティング
  - [x] argcとenvpのサポート
  - [x] pipeのサポート

### 新規追加機能

- [x] 二重間接ブロックによる約8.5MBまでの大規模ファイルに対応
- [x] 時間機能のサポート
  - [x] RTC (DS3231)対応
  - [x] タイマー対応
- [x] シグナル機能のサポート
- [x] mmap機能のサポート
- [x] ファイル情報のサポート
- [x] ユーザ情報のサポート
- [x] システムコールの追加（23 -> 83)
- [x] Dashのサポート
- [x] Coreutilsのサポート
- [x] login, passwd, suコマンドのサポート
- [x] Dynamic Link機能を実装
- [x] Binutilsをdynamic linkでビルドして導入
- [x] ファイル読み込みにpagecacheを使用

## 前提条件

x86で動くUbun20 20.04では`make init`を実行するだけで、このセクションは飛ばしてください。

### GCCツールチェイン
macOSでクロスコンパイルする場合は次の手順に従ってください。

1. `brew install zstd`を実行する
2. [macos-cross-toolchains](https://github.com/messense/homebrew-macos-cross-toolchains)からaarch64-unknown-linux-gnuをインストールする
3. config.mkで`CROSS := aarch64-unknown-linux-gnu-`のようにCROSS変数を変更する

ネイティブコンパイルする場合、すなわち、ARMv8上でネイティブに実行する場合は、config.mkで`CROSS := `と設定し、ローカルのgccを使ってください。

If `$(CROSS)gcc --version`を実行してバージョンが9.3.0未満の場合はMakefileの`-mno-outline-atomics`を削除してください。

### QEMU

QEMU (>= 6.0)をパッケージマネージャからインストールするか、次の手順でソースからコンパイルしてください。

```
git clone https://github.com/qemu/qemu.git
mkdir -p qemu/build
(cd qemu/build && ../configure --target-list=aarch64-softmmu && make -j8)
```

CentOSなど位一部のOSでは次のパッケージのインストールも必要かもしれません。

```
yum install ninja-build
yum install pixman-devel.aarch64
```

作成された`qemu-system-aarch64`をPATHに追加するか、config.mkの`QEMU`変数を変更してください。

### muslのビルド

まず、 `git submodule update --init --recursive`を実行してmuslを取り込みます。

次に、クロスコンパイルする場合は`(cd libc && export CROSS_COMPILE=X && ./configure --target=aarch64)`を実行します。ここで、`X`はconfig.mkの`CROSS`変数の値です。それ以外の場合は`(cd libc && ./configure)`を実行します。

### ファイルシステムツール

fat32ファイルシステムイメージをビルドするためにmtoolsを使用しています。これはパッケージマネージャでインストールすることができます。たとえば、Ubunntuでは`sudo apt install mtools`を、macOSでは`brew install mtools`を実行してください。

MBRパーティションディスクイメージを作成するためにsfdiskを使用しています。このコマンドはほとんどのLinuxディストリビューションにはすでにインストールされていますが、macOSの場合は次の手順でインストールしてください。

1. `brew install util-linux`を実行してsfdiskをインストールする
2. brewの指示に従いPATHを変更する

## 開発

- `make qemu`: `obj/kernel8.img`にあるカーネルをエミュレートする
- `make`: Raspberry Pi 3用のブート可能なSDカードイメージを`obj/sd.img`に作成する。 これは[Raspberry Pi Imager](https://www.raspberrypi.org/software/)を使ってtfに焼くことができます。
- `make lint`: カーンルコードとユーザプログラムの構文チェックをします。

### Raspberry Pi 4

Pi 4でも正常に動きます。Makefileの`RASPI := 3`を`RASPI := 4`に変更して、`make clean && make`を実行し、Pi 4で楽しんでください。

### ログレベル

ログレベルはMakefileにあるコンパイラオプション`-DLOG_XXX`で制御されます。ここで`XXX`に指定できるのは以下のいずれかです。

- `ERROR`
- `WARN`
- `INFO`
- `DEBUG`
- `TRACE`

デフォルトは`-DLOG_INFO`です。

### デバッグモード

Makefileにあるコンパイラオプション`-DDEBUG`でデバッグモードを有効にすると、実行時のアサーション、テスト、メモリ使用量プロファイリング（下記参照）が組み込まれます()。デフォルトは `-DNOT_DEBUG` です。

### プロファイル

`Ctrl+P`と押下することでプロセスとメモリ使用量（`-DDEBUG`を有効にした場合に表示）に関する情報を見ることができます。

次のように出力されます。

```
1 sleep  init
2 run    idle
3 runble idle
4 run    idle
5 run    idle
6 sleep  sh fa: 1
```

ここで、各行はプロセスのpid, 状態、親のpidです。

## プロジェクトの構成

```
.
├── Makefile
├── mksd.mk: ブート可能なイメージを生成するためのMakefikeの構成ファイル
|
├── boot: オフィシャルブートローダ
├── libc: Cライブラリmusl.
|
├── inc: カーネルヘッダ
├── kern: カーネルソースコード
└── usr: ユーザプログラム
```
