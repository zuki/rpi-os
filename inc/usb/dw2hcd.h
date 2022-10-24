//
// From dwhcidevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright(C) 2014-2022  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// DesignWave USB2 Host Controller device
#ifndef H_DWHCD_H
#define H_DWHCD_H

#include "usb/usb.h"
#include "usb/endpoint.h"
#include "usb/request.h"
#include "usb/dw2rootport.h"
#include "usb/dw2xferstagedata.h"
#include "usb/dw2hc.h"
#include "usb/standardhub.h"
#include "list.h"
#include "types.h"
#include "spinlock.h"
#include "synchronize.h"

#define DWHCI_WAIT_BLOCKS   DWHCI_MAX_CHANNELS

/// @brief DesignWave USB2 ホストコントローラ構造体
typedef struct dw2_hc {
    unsigned            channels;               ///< チャネル数
    volatile unsigned   allocch;                ///< 割り当て済みチャネル数: 1ビット1チャネル
    struct spinlock     chanlock;                ///< スピンロック: チャネルを保護
    struct spinlock     wblklock;               ///< スピンロック: wait blockを保護
    struct spinlock     imasklock;              ///< スピンロック: 割り込みマスクを保護
    dw2_xfer_stagedata_t stdata[DWHCI_MAX_CHANNELS]; ///< 転送ステートデータ
    volatile boolean    waiting[DWHCI_WAIT_BLOCKS];         ///< 待機中
    volatile unsigned   allocbits;              ///< 割り当て済みウェイトブロック: 1ビット1wait block
    dw2_rport_t         rport;                  ///< ルートポート
    volatile boolean    rpenabled;              ///< ルートポートが有効か
    volatile boolean    shutdown;               ///< USBドライバはshutdownするか

#ifdef USE_USB_SOF_INTR
    struct list_head   *tqueue;                 ///< トランザクションキュー
#endif
// 以下、usbhostcontrollerから
    struct spinlock     hublock;                ///< スピンロック: ハブリストを保護
    struct list_head   *hublist;                ///< ハブリスト
    /* boolean             pap; */              ///< プラグアンドプレイは有効か
} dw2_hc_t;

/// @brief ポートステータスイベント構造体
typedef struct port_status_event {
    boolean              fromrp;  // ルートハブからか
    union {
        dw2_rport_t     *rport;
        usb_stdhub_t    *hub;
    };
    struct list_head     list;
} pst_ev_t;

// PUBLIC
void dw2_hc(dw2_hc_t *self);
void _dw2_hc(dw2_hc_t *self);

/// @brief DWHCデバイスの初期化
/// @param scan 初期化の最後にデバイスを再スキャンするか
/// @return 成功したらtrue, 失敗したらfalse
boolean dw2_hc_init(dw2_hc_t *self, int scan);

/// @brief ルートポートが初期化されていなかったら初期化してUSBデバイスが接続されていないかスキャン
void dw2_hc_rescan_dev(dw2_hc_t *self);

/// @brief リクエストをブロッキングモードで発行する（アイソクロナス転送は未サポート）
/// @param urb リクエスト
/// @param timeout タイムアウト時間（デフォルトは0: なし）
/// @return true, 失敗したらfalse
boolean dw2_hc_submit_block_request(dw2_hc_t *self, usb_req_t *urb, unsigned timeout);

/// @brief リクエストの非同期転送
/// @param urb USBリクエストオブジェクト
/// @param timeout タイムアウト時間（デフォルトは0: なし）
/// @return 操作の成否
boolean dw2_hc_submit_async_request(dw2_hc_t *self, usb_req_t *urb, unsigned timeout);

/// @brief 指定デバイスのトランザクションをキャンセルする
/// @param device キャンセルするデバイス
// USE_USB_SOF_INTRの場合のみ使用する関数で未実装
//void dw2_hc_cancel_device_transaction(dw2_hc_t *self, usb_dev_t *dev);

// static関数

/// @brief ホストポートはコネクトされているか
/// @return されていればtrue, されていなければfalse
boolean dw2_hc_device_connected(dw2_hc_t *self);

// @brief ホストポートのスピードを取得する
/// @return ホストスピード
usb_speed_t dw2_hc_get_port_speed(dw2_hc_t *self);

/// @brief 過電流がないか
/// @return 過電流があればTRUE
boolean dw2_hc_overcurrent_detected(dw2_hc_t *self);

/// @brief ホストのルートポートを無効化して、電源を切る（bPowerOff = TRUEの場合）
/// @param poweroff 電源を切るか
void dw2_hc_disable_rport(dw2_hc_t *self, boolean poweroff);

/// @brief コアの初期化
/// @return 操作の成否
boolean dw2_hc_init_core(dw2_hc_t *self);

/// @brief dwhcホストの初期化
/// @return 常にTRUE
boolean dw2_hc_init_host(dw2_hc_t *self);

/// @brief ルートポートの有効化
/// @return 操作の成否
boolean dw2_hc_enable_rport(dw2_hc_t *self);

/// @brief dwhcホストの電源オン
/// @return 操作の成否
boolean dw2_hc_power_on(dw2_hc_t *self, uint32_t poweron);

/// @brief dwhcホストのリセット
/// @return 操作の成否
boolean dw2_hc_reset(dw2_hc_t *self);

/// @brief グローバル割り込みの有効化
void dw2_hc_enable_global_intr(dw2_hc_t *self);

/// @brief 一般割り込みの有効化
void dw2_hc_enable_common_intr(dw2_hc_t *self);

/// @brief dwhcホスト割り込みの有効化
void dw2_hc_enable_host_intr(dw2_hc_t *self);

/// @brief チャネル割り込みの有効化
/// @param channel 有効にするチャネル番号
void dw2_hc_enable_channel_intr(dw2_hc_t *self, unsigned channel);

/// @brief チャネル割り込みの無効化
/// @param channel 無効にするチャネル番号
void dw2_hc_disable_channel_intr(dw2_hc_t *self, unsigned channel);

/// @brief 送信FIFOのフラッシュ
/// @param nFIFO FIFO番号
void dw2_hc_flush_tx_fifo(dw2_hc_t *self, unsigned fifo);

/// @brief 受信FIFOのフラッシュ
void dw2_hc_flush_rx_fifo(dw2_hc_t *self);

/// @brief リクエストの転送
/// @param urb USBリクエストオブジェクト
/// @param in INリクエストか
/// @param stage  ステータスステージか
/// @param timeout タイムアウト
/// @return
boolean dw2_hc_xfer_stage(dw2_hc_t *self, usb_req_t *urb, boolean in, boolean stage, unsigned timeout);

/// @brief 転送完了処理
/// @param urb リクエスト
/// @param param リクエストパラメタ
/// @param ctx コンテキスト
void dw2_hc_comp_cb(usb_req_t *urb, void *param, void *ctx);

// @brief リクエストの非同期転送
/// @param urb USBリクエストオブジェクト
/// @param in INリクエストか
/// @param stage ステータスステージか
/// @param timeout タイムアウト
/// @return
boolean dw2_hc_xfer_stage_async(dw2_hc_t *self, usb_req_t *urb, boolean in, boolean stage, unsigned timeout);

#ifdef USE_USB_SOF_INTR
/// @brief トランザクションをキューに入れる
/// @param data トランザクションステージデータ
void dw2_hc_queue_trans(dw2_hc_t *self, dw2_xfer_stagedata_t *data);
/// @brief 遅延を入れてトランザクションをキューに入れる
/// @param data トランザクションステージデータ
void dw2_hc_queue_delay_trans(dw2_hc_t *self, dw2_xfer_stagedata_t *data);
#endif

/// @brief トランザクションを開始する
/// @param data トランザクションステージデータ
void dw2_hc_start_trans(dw2_hc_t *self, dw2_xfer_stagedata_t *stdata);
/// @brief チャネルを開始する
/// @param data トランザクションステージデータ
void dw2_hc_start_channel(dw2_hc_t *self, dw2_xfer_stagedata_t *stdata);
/// @brief チャネル割り込みハンドラ
/// @param channel チャネル番号
void dw2_hc_channel_intr_hdl(dw2_hc_t *self, unsigned channel);
#ifdef USE_USB_SOF_INTR
/// @brief SOF割り込みハンドラ
void dw2_hc_sof_intr_hdl(dw2_hc_t *self);
#endif
/// @brief 割り込みハンドラ
void dw2_hc_intr_hdl(dw2_hc_t *self);
/// @brief 割り込みスタブ
/// @param param パラメタ（dwhciデバイスへのポインタ）
void dw2_hc_intr_hdlstub(void *param);

#ifndef USE_USB_SOF_INTR
    void dw2_hc_timer_hdl(dw2_hc_t *self, dw2_xfer_stagedata_t *stdata);
    void dw2_hc_timer_hdlstub(uint64_t data);
#endif
/// @brief チャネルを割り当てる
/// @return チャネル番号
unsigned dw2_hc_alloc_channel(dw2_hc_t *self);
/// @brief チェネルを開放する
/// @param channel チャネル番号
void dw2_hc_free_channel(dw2_hc_t *self, unsigned channel);
/// @brief waitblockを割り当てる
/// @return waitblock
unsigned dw2_hc_alloc_wblock(dw2_hc_t *self);
/// @brief waitblockを開放する
/// @param wblock 待機ブロック
void dw2_hc_free_wblock(dw2_hc_t *self, unsigned wblock);

/// @brief 指定のビットが立つのを待つ
/// @param register 対象レジスタ
/// @param mask 対象ビット
/// @param bit 待機ビット
/// @param timeout タイムアウト
/// @return ビットが立ったらTRUE, タイムアウトになったらFALSE
boolean dw2_hc_wait_for_bit(dw2_hc_t *self, uint64_t reg, uint32_t mask, boolean bit, unsigned timeout);

/// @brief レジスタ値をダンプ
/// @param name レジスタ名
/// @param addr レジスタアドレス
void dw2_hc_dumpreg(dw2_hc_t *self,const char *name, uint32_t addr);
/// @brief ステータスをダンプ
/// @param channel チャネル番号
void dw2_hc_dump_status (dw2_hc_t *self, unsigned channel);

// 以下は、usbhostcontrollerから

/// @brief ディスクリプタを取得する
/// @param ep エンドポイント
/// @param type ディスクリプタのタイプ
/// @param index インデックス
/// @param buffer バッファ
/// @param buflen バッファのサイズ
/// @param reqtype リクエストタイプ
/// @param wIndex エンドポイント(0)、インタフェース、言語IDのいずれか
/// @return 長さ、失敗した場合は負値
int dw2_hc_get_desc(dw2_hc_t *self, usb_ep_t *ep,
                  unsigned char type, unsigned char index,
                  void *buffer, unsigned buflen,
                  unsigned char reqtype, unsigned short idx);

/// @brief デバイスアドレスをセット
/// @param ep エンドポイント
/// @param addr デバイスアドレス
/// @return 成功したらTRUE, 失敗したらFALSE
boolean dw2_hc_set_addr(dw2_hc_t *self, usb_ep_t *ep, uint8_t addr);

///@brief コンフィグレーションをセット
/// @param ep エンドポイント
/// @param cfgvalue コンフィグレーション値
/// @return 成功したらTRUE, 失敗したらFALSE
boolean dw2_hc_set_config(dw2_hc_t *self, usb_ep_t *ep, uint8_t cfgvalue);

/// @brief メッセージを送信する
/// @param ep エンドポイント
/// @param reqtype リクエストタイプ
/// @param req リクエスト
/// @param value 値
/// @param index インデックス
/// @param data データ
/// @param datalen データサイズ
/// @return 長さ、失敗した場合は負値
int dw2_hc_control_message(dw2_hc_t *self, usb_ep_t *ep,
            uint8_t reqtype, uint8_t req, uint16_t value, uint16_t index,
            void *data, uint16_t datalen);

/// @brief 転送を行う
/// @return 実際に送信した長さ、失敗の場合は負値
int dw2_hc_xfer(dw2_hc_t *self, usb_ep_t *ep, void *buffer, unsigned buflen);

/* plug and play関連
static inline boolean dw2_hc_is_pap(dw2_hc_t *self) {
    return self->pap;
}

/// @brief デバイスツリーが更新されたか（TASK_LEVELで呼び出す必要がある）
/// @return プラグアンドプレイが有効な場合、デバイスツリーが更新された場合にtrue返す。\n
/// （最初に呼び出されたときは常にtrueを返す）
boolean dw2_hc_update_pap(dw2_hc_t *self);
*/

void dw2_hc_dumreg(dw2_hc_t *self, const char *name, uint64_t addr);
void dw2_hc_dump_status(dw2_hc_t *self, unsigned channel);

#endif
