# Raspberry Pi ImagerによるSDカードへの書き込み

## 実機用に再コンパイルする

1. `inc/rtc.h`で`#undef USING_RASPI`行をコメントアウトし、`#define USING_RASPI 1`行のコメントを外す
2. `rm -rf obj && make`

## Raspberry Pi ImagerでSDカードを作成する

### 1. Raspberry Pi Imagerを立ち上げる

<center>

![Raspberry Pi Imager](images/icon.png)

</center>

### 2. トップ画面で[OSを選ぶ]ボタンをクリック

<center>

![OSを選ぶボタン](images/os_button.png)

</center>

### 4. OS選択画面で[カスタムイメージを使う]を選択する

<center>

![OS選択画面画面](images/select_os.png)

</center>

### 4. ファイル選択画面で`obj/sd.img`を選択する

<center>

![ファイル選択画面画面](images/select_sd.png)

</center>

### 5. トップ画面で[ストレージを選ぶ]ボタンをクリック

<center>

![ストレージを選ぶボタン](images/storage_button.png)

</center>

### 6. 書き込むSDカードを選択

<center>

![SDカード選択画面](images/select_card.png)

</center>

### 5. トップ画面で[書き込む]ボタンをクリック

<center>

![書き込むボタン](images/write_button.png)

</center>

### 6. 書き込みが終わったらSDカードを取り出し、RasPiに挿入
