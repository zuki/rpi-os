//
// dw2_hc_.c
//
// Supports:
//    internal DMA only,
//    no ISO transfers
//    no dynamic attachments
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright(C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include "usb/dw2hcd.h"
#include "usb/usb.h"
#include "usb/request.h"
#include "usb/dw2xferstagedata.h"
#include "usb/dw2fscheduler.h"
#include "usb/dw2rootport.h"
#include "usb/devicenameservice.h"
#include "types.h"
#include "arm.h"
#include "console.h"
#include "irq.h"
#include "kmalloc.h"
#include "list.h"
#include "mbox.h"
#include "synchronize.h"
#include "linux/time.h"
#include "proc.h"
#include "rtc.h"

#define DEVICE_ID_USB_HCD       3           // for SetpowerStateOn()
#define POWER_STATE_OFF         (0 << 0)
#define POWER_STATE_ON          (1 << 0)
#define POWER_STATE_WAIT        (1 << 1)
#define POWER_STATE_NO_DEVICE   (1 << 1)    // in response

// Dw2＿HCDドライバオブジェクト
static dw2_hc_t *dw2hc = 0;

//
// 構成
//
#define DWC_CFG_DYNAMIC_FIFO        // 次のサイズでFIFOを再プログラム
    #define DWC_CFG_HOST_RX_FIFO_SIZE       1024    // 32ビットワードの数
    #define DWC_CFG_HOST_NPER_TX_FIFO_SIZE  1024    // 32ビットワードの数
    #define DWC_CFG_HOST_PER_TX_FIFO_SIZE   1024    // 32ビットワードの数

#define MSEC2HZ(msec)       ((msec) * HZ / 1000)

/// @brief ステージステータス
typedef enum {
    stage_status_no_split,
    stage_status_start_split,
    stage_status_complete_split,
    stage_status_periodic_delay,
    stage_status_unknown
} stage_status_t;

/// @brief ステージサブステータス
typedef enum {
    stage_substate_wait_for_channel_disaable,
    stage_substate_wait_for_xfer_complete,
    stage_substate_unknown
} stage_substate_t;

unsigned dw2_hc_allocate_channel(dw2_hc_t *self);
void dw2_hc_free_channel(dw2_hc_t *self, unsigned channel);
//boolean dw2_hc_wait_for_bit(dw2_hc_t *self, uint32_t reg, uint32_t mask,boolean wait, unsigned timeout);
void dw2_hc_dump_register(dw2_hc_t *self, const char *name, uint32_t addr);
void dw2_hc_dump_status(dw2_hc_t *self, unsigned channel);

typedef void(*fn)(uint64_t);

static void
start_timer(uint64_t expires, fn handler, void *param, void *context)
{
    struct timer_list *timer = (struct timer_list *)kmalloc(sizeof(struct timer_list));
    uint64_t *params = (uint64_t *)kmalloc(2 * sizeof(uint64_t));
    params[0] = (uint64_t)param;
    params[1] = (uint64_t)context;
    init_timer(timer);
    timer->expires = expires;
    timer->data = (uint64_t)params;
    timer->function = handler;
    add_timer(timer);
}

void dw2_hc(dw2_hc_t *self)
{
    self->channels = 0;
    self->allocch = 0;
    self->allocwblk = 0;
    self->rpenabled = false;
    self->shutdown = false;
    initlock(&self->chanlock, "chanlock");
    initlock(&self->wblklock, "waitblocklock");
    initlock(&self->imasklock, "itrmasklock");
    initlock(&self->hublock, "hublock");
    list_init(&self->hublist);
#ifdef USE_USB_SOF_INTR
    list_init(&self->tqueue);
#endif
    //self->pap = false;
    dw2_rport(&self->rport, self);

    for (unsigned ch = 0; ch < DWHCI_MAX_CHANNELS; ch++) {
        self->stdata[ch] = 0;
    }

    for (unsigned blk = 0; blk < DWHCI_WAIT_BLOCKS; blk++) {
        self->waiting[blk] = false;
    }
    trace("dw2_hc created");
}

void _dw2_hc(dw2_hc_t *self)
{
    self->shutdown = true;
    delayus(200*1000);      // 全トランザクションの完了を待つ

    irq_disable(IRQ_USB_HC);

    dw2_hc_reset(self);

    // mboxを使用
    if (!dw2_hc_power_on(self, POWER_STATE_OFF)) {
        warn("failed power off");
    }

    _dw2_rport(&self->rport);
}

boolean dw2_hc_init(dw2_hc_t *self, boolean scan)
{
    uint32_t vendor;
    // 1. ホストコントローラのバージョンチェック
    // この値はDWC OTG HW Release version。実機は0x4f54280a、QEMUは0x4f54294a
    // 両者のminorバージョンが異なるのでmajorバージョンのみチェックする
    if (((vendor = get32(DWHCI_CORE_VENDOR_ID)) & 0xfffff000) != 0x4f542000) {
        warn("Unknown vendor 0x%x", vendor);
        return false;
    }
    // 2. 電源を入れる（MBOX経由）
    if (!dw2_hc_power_on(self, POWER_STATE_ON)) {
        warn("Cannot power on ox%x", vendor);
        return false;
    }
    // 3. すべての割り込みを無効にする
    uint32_t ahbcfg;
    ahbcfg = get32(DWHCI_CORE_AHB_CFG);
    ahbcfg &= ~DWHCI_CORE_AHB_CFG_GLOBALINT_MASK;
    put32(DWHCI_CORE_AHB_CFG, ahbcfg);

    // 4. USB HCへの割り込みを有効化してハンドラを登録
    irq_enable(IRQ_USB_HC);
    irq_register(IRQ_USB_HC, dw2_hc_intr_hdlstub, self);

    // 5. コアを初期化
    if (!dw2_hc_init_core(self)) {
        warn("Cannot initialize core");
        return false;
    }
    // 6. 割り込みを有効にする
    dw2_hc_enable_global_intr(self);
    // USBホストを初期化
    if (!dw2_hc_init_host(self)) {
        warn("Cannot initialize host");
        return false;
    }
/*
    // 以下の呼び出しはルートポートにデバイスが接続されていないと
    // 失敗する。システムはUSBデバイスなしでも実行できるのでこれは
    // エラーではない。
    if (!dw2_hc_enable_rport(self)) {
        warn("No device connected to root port");
        return true;
    }
    self->rpenabled = true;

    if (!dw2_rport_init(&self->rport)) {
        warn("Cannot initialize root port");
        return true;
    }
*/

    DataMemBarrier();

    if (scan) {
        dw2_hc_rescan_dev(self);
    }

    return true;
}

void dw2_hc_rescan_dev(dw2_hc_t *self)
{
    if (!self->rpenabled) {
            // ルートポートの有効化
        if (dw2_hc_enable_rport(self)) {
            self->rpenabled = true;
            if (!dw2_rport_init(&self->rport)) {
                warn("Cannot initialize root port");
            }
        } else {
            warn("No device connected to root port");
        }
    } else {
        dw2_rport_rescan_dev(&self->rport);
    }
}

// ok
boolean dw2_hc_submit_block_request(dw2_hc_t *self, usb_req_t *urb, unsigned timeout)
{
    trace("host=0x%p, urb=0x%p, timeout=%d", self, urb, timeout);
    DataMemBarrier();

    // 1. リクエストステータスを初期セット
    usb_req_set_status(urb, 0);

    // 2. コントロール転送
    if (urb->ep->type == ep_type_control) {
        assert(timeout == USB_TIMEOUT_NONE);
        setup_data_t *setup = urb->setup_data;
        // 2-1. INリクエストの場合
        if (setup->reqtype & REQUEST_IN) {
            // 2-1-1. セットアップステージ: INコマンドの送信
            trace("Control xfer: IN request");
            if (!dw2_hc_xfer_stage(self, urb, false, false, USB_TIMEOUT_NONE)
            // 2-1-2. データステージ: デバイスからデータの受信
             || !dw2_hc_xfer_stage(self, urb, true,  false, USB_TIMEOUT_NONE)
            // 2-1-3. ステータスステージ: OUT ACK
             || !dw2_hc_xfer_stage(self, urb, false, true, USB_TIMEOUT_NONE)) {
                warn("failed out command to get data");
                return false;
            }
        // 2-2. OUTリクエストの場合
        } else {
            // 2-2-1. データを受け取らない場合
            if (usb_req_get_buflen(urb) == 0) {
                trace("Control xfer: OUT request no data");
                // 2-2-1-1. セットアップステージ: OUTコマンドの送信
                if (!dw2_hc_xfer_stage(self, urb, false, false, USB_TIMEOUT_NONE)
                // 2-2-1-2. ステータスステージ: IN ACK
                 || !dw2_hc_xfer_stage(self, urb, true,  true, USB_TIMEOUT_NONE)) {   // in ack
                    warn("failed out request");
                    return false;
                }
            // 2-2-1. データを受け取る場合
            } else {
                trace("Control xfer: OUT request with data");
                // 2-2-2-1. セットアップステージ: OUTコマンドの送信
                if (!dw2_hc_xfer_stage(self, urb, false, false, USB_TIMEOUT_NONE)
                // 2-2-2-2. データステージ: デバイスへのデータの送信
                 || !dw2_hc_xfer_stage(self, urb, false, false, USB_TIMEOUT_NONE)
                // 2-2-2-3. ステータスステージ: IN ACK
                 || !dw2_hc_xfer_stage(self, urb, true,  true, USB_TIMEOUT_NONE)) {
                    warn("failed out data");
                    return false;
                }
            }
        }
    // 3. バルク/インタラプト転送（アイソクロナス転送は未サポート）
    } else {
        trace("Bulk/Interrupt xfer");
        if (!dw2_hc_xfer_stage(self, urb, urb->ep->in, false, timeout)) {
            warn("failed bulk or interrupt xter");
            return false;
        }
    }
    DataMemBarrier();
    return true;
}

boolean dw2_hc_submit_async_request(dw2_hc_t *self, usb_req_t *urb, unsigned timeout)
{
    DataMemBarrier();
    usb_req_set_status(urb, 0);
    boolean result = dw2_hc_xfer_stage_async(self, urb, urb->ep->in, false, timeout);
    DataMemBarrier();
    return result;
}


void dw2_hc_cancel_device_transaction(dw2_hc_t *self, usb_dev_t *dev)
{
#ifdef USE_USB_SOF_INTR
    // dw2_hc_flush_dev(self, dev);     // 未定義、未実装
#endif
}


boolean dw2_hc_device_connected(dw2_hc_t *self)
{
    uint32_t hostport = get32(DWHCI_HOST_PORT);
    return hostport & DWHCI_HOST_PORT_CONNECT ? true : false;
}

usb_speed_t dw2_hc_get_port_speed(dw2_hc_t *self)
{
    usb_speed_t speed = usb_speed_unknown;

    uint32_t hostport = get32(DWHCI_HOST_PORT);
    switch(DWHCI_HOST_PORT_SPEED(hostport)) {
        case DWHCI_HOST_PORT_SPEED_HIGH:
            speed = usb_speed_high;
            break;
        case DWHCI_HOST_PORT_SPEED_FULL:
            speed = usb_speed_full;
            break;
        case DWHCI_HOST_PORT_SPEED_LOW:
            speed = usb_speed_low;
            break;
        default:
            break;
    }
    return speed;
}

boolean dw2_hc_overcurrent_detected(dw2_hc_t *self)
{
    if (get32(DWHCI_HOST_PORT) & DWHCI_HOST_PORT_OVERCURRENT)
        return true;

    return false;
}

void dw2_hc_disable_rport(dw2_hc_t *self, boolean poweroff)
{
    self->rpenabled = false;

    uint32_t hostport = get32(DWHCI_HOST_PORT);
    hostport &= ~DWHCI_HOST_PORT_POWER;
    if (poweroff) {
        hostport &= ~DWHCI_HOST_PORT_POWER;
    }
    put32(DWHCI_HOST_PORT, hostport);
#ifdef USE_USB_SOF_INTR
    // m_TransactionQueue.Flush (); 未実装
#endif
}

/// @brief コアの初期化
/// @param self USBホストコントローラ
/// @return 操作の成否
boolean dw2_hc_init_core(dw2_hc_t *self)
{
    // 1. USB, USB PHYの構成1
    uint32_t usbcfg;    // USB Configuration Register: USB, USB PHYの構成
    usbcfg = get32(DWHCI_CORE_USB_CFG);
    usbcfg &= ~DWHCI_CORE_USB_CFG_ULPI_EXT_VBUS_DRV;    // default
    usbcfg &= ~DWHCI_CORE_USB_CFG_TERM_SEL_DL_PULSE;    // default
    put32(DWHCI_CORE_USB_CFG, usbcfg);

    // 2. コアをソフトリセット
    if (!dw2_hc_reset(self)) {
        warn("reset failed");
        return false;
    }

    // 3. USB, USB PHYの構成2
    usbcfg = get32(DWHCI_CORE_USB_CFG);
    usbcfg &= ~DWHCI_CORE_USB_CFG_ULPI_UTMI_SEL;    // select UTMI+
    usbcfg &=  ~DWHCI_CORE_USB_CFG_PHYIF;           // UTMI width is 8
    put32(DWHCI_CORE_USB_CFG, usbcfg);

    // 4. 内部DMAモードであることを確認
    uint32_t hwcfg2;
    hwcfg2 = get32(DWHCI_CORE_HW_CFG2);
    assert(DWHCI_CORE_HW_CFG2_ARCHITECTURE(hwcfg2) == 2);   // [4:3] = 2

    // 5. USB, USB PHYの構成3
    usbcfg = get32(DWHCI_CORE_USB_CFG);
    // 5-1-1 [7:6] = 2 : HS PHYインタフェースタイプがULPI
    // 5-1-2 [9:8] = 1 : FS PHYインタフェースタイプがFS専用インタフェース
    if (DWHCI_CORE_HW_CFG2_HS_PHY_TYPE(hwcfg2) == DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
     && DWHCI_CORE_HW_CFG2_FS_PHY_TYPE(hwcfg2) == DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED) {
        usbcfg |= DWHCI_CORE_USB_CFG_ULPI_FSLS;         // [17]=1: ULPI FS/LS serial interface
        usbcfg |= DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M;    // [19]=1: サスペンド時にも内部クロックの電源を落とさない
    } else {
        usbcfg &= ~DWHCI_CORE_USB_CFG_ULPI_FSLS;        // [17]=0: ULPI interface
        usbcfg &= ~DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M;   // [19]=0: 電源を落とす
    }
    put32(DWHCI_CORE_USB_CFG, usbcfg);

    // 6. ホストのチャネル数の設定: [17:14] + 1 (0が1を表すため+1): 4<= channels <= 16
    self->channels = DWHCI_CORE_HW_CFG2_NUM_HOST_CHANNELS(hwcfg2);
    assert(4 <= self->channels && self->channels <= DWHCI_MAX_CHANNELS);

    // 7. AHBの構成
    uint32_t ahbcfg;
    ahbcfg = get32(DWHCI_CORE_AHB_CFG);
    ahbcfg |= DWHCI_CORE_AHB_CFG_DMAENABLE;     // [5]=1 : コア操作をDMAモードで行う
    // ahbcfg |= DWHCI_CORE_AHB_CFG_AHB_SINGLE; // if DMA single mode should be used
    ahbcfg |= DWHCI_CORE_AHB_CFG_WAIT_AXI_WRITES;   // [2:1] = 0 : Burst type = Single
    ahbcfg &= ~DWHCI_CORE_AHB_CFG_MAX_AXI_BURST__MASK;
    // ahbcfg |= (0 << DWHCI_CORE_AHB_CFG_MAX_AXI_BURST__SHIFT) // max. AXI burst length 4
    put32(DWHCI_CORE_AHB_CFG, ahbcfg);

    // 9. USB, USB PHYの構成4: HNPとSRPは使わない
    usbcfg = get32(DWHCI_CORE_USB_CFG);
    usbcfg &= ~DWHCI_CORE_USB_CFG_HNP_CAPABLE;  // [9]=0 : HNPを無効化
    usbcfg &= ~DWHCI_CORE_USB_CFG_SRP_CAPABLE;  // [8]=0 : SRPを無効化
    put32(DWHCI_CORE_USB_CFG, usbcfg);

    // 10. システムレベルの割り込みをすべて有効化
    dw2_hc_enable_common_intr(self);

    return true;
}

boolean dw2_hc_init_host(dw2_hc_t *self)
{
    // PHYクロックをリスタート
    uint32_t power = 0;
    put32(ARM_USB_POWER, power);

    uint32_t hostcfg;
    hostcfg = get32(DWHCI_HOST_CFG);
    hostcfg &= ~DWHCI_HOST_CFG_FSLS_PCLK_SEL__MASK;

    uint32_t hwcfg2;
    hwcfg2 = get32(DWHCI_CORE_HW_CFG2);
    uint32_t usbcfg;
    usbcfg = get32(DWHCI_CORE_USB_CFG);
    if (DWHCI_CORE_HW_CFG2_HS_PHY_TYPE(hwcfg2) == DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
     && DWHCI_CORE_HW_CFG2_FS_PHY_TYPE(hwcfg2) == DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED
     && (usbcfg & DWHCI_CORE_USB_CFG_ULPI_FSLS)) {
        hostcfg |= DWHCI_HOST_CFG_FSLS_PCLK_SEL_48_MHZ;
    } else {
        hostcfg |= DWHCI_HOST_CFG_FSLS_PCLK_SEL_30_60_MHZ;
    }

    put32(DWHCI_HOST_CFG, hostcfg);
#ifdef DWC_CFG_DYNAMIC_FIFO
    uint32_t rxfifosize = DWC_CFG_HOST_RX_FIFO_SIZE;
    put32(DWHCI_CORE_RX_FIFO_SIZ, rxfifosize);

    uint32_t nptxfilosize = 0;
    nptxfilosize |= DWC_CFG_HOST_RX_FIFO_SIZE;
    nptxfilosize |= (DWC_CFG_HOST_NPER_TX_FIFO_SIZE << 16);
    put32(DWHCI_CORE_NPER_TX_FIFO_SIZ, nptxfilosize);

    uint32_t hptxfifosize = 0;
    hptxfifosize |= (DWC_CFG_HOST_RX_FIFO_SIZE + DWC_CFG_HOST_NPER_TX_FIFO_SIZE);
    hptxfifosize |= (DWC_CFG_HOST_PER_TX_FIFO_SIZE << 16);
    put32(DWHCI_CORE_HOST_PER_TX_FIFO_SIZ, hptxfifosize);
#endif

    dw2_hc_flush_tx_fifo(self, 0x10);       // Flush all TX FIFOs
    dw2_hc_flush_rx_fifo(self);
    uint32_t hostport;
    hostport = get32(DWHCI_HOST_PORT);
    hostport &= ~DWHCI_HOST_PORT_DEFAULT_MASK;
    if (!(hostport & DWHCI_HOST_PORT_POWER)) {
        hostport |= DWHCI_HOST_PORT_POWER;
        put32(DWHCI_HOST_PORT, hostport);
    }
    dw2_hc_enable_host_intr(self);
    return true;
}

boolean dw2_hc_enable_rport(dw2_hc_t *self)
{
    uint32_t hostport;
    // ホストポートを有効化
    if (!dw2_hc_wait_for_bit(self, DWHCI_HOST_PORT, DWHCI_HOST_PORT_CONNECT, true, 510)) {
        warn("host port busy");
        return false;
    }
    delayus(100*1000);      // see USB 2.0 spec
    // ホストポートをリセット
    hostport = get32(DWHCI_HOST_PORT);
    hostport &= ~DWHCI_HOST_PORT_DEFAULT_MASK;
    hostport |= DWHCI_HOST_PORT_RESET;
    put32(DWHCI_HOST_PORT, hostport);

    delayus(50*1000);       // see USB 2.0 spec(tDRSTR)
    // リセットリカバリ
    hostport = get32(DWHCI_HOST_PORT);
    hostport &= ~DWHCI_HOST_PORT_DEFAULT_MASK;
    hostport &= ~DWHCI_HOST_PORT_RESET;
    put32(DWHCI_HOST_PORT, hostport);

    // 通常は10msだが、それでは短すぎるデバイスがあるようだ
    delayus(20*1000);       // see USB 2.0 spec(tRSTRCY)

    trace("enabled rootport");
    return true;
}

/// @brief USBホストコントローラの電源オン（MBOXを使用）
/// @param self USBホストコントローラ
/// @param poweron オン(1)/オフ(0)
/// @return 操作の成否
boolean dw2_hc_power_on(dw2_hc_t *self, uint32_t poweron)
{
    int result = mbox_set_power_state(DEVICE_ID_USB_HCD, poweron, POWER_STATE_WAIT);

    if (poweron) {
        return result == 1 ? true : false;
    } else {
        return result == 0 ? true : false;
    }
}

/// @brief USBホストコントローラをリセットする
/// @param self USBホストコントローラ
/// @return 操作の成否
boolean dw2_hc_reset(dw2_hc_t *self)
{
    uint32_t reset = 0;
    // 1. AHBマスタがIDLE状態になるのを待つ: GRSTCTL: [31] = 1 （reset時のデフォルト）
    if (!dw2_hc_wait_for_bit(self, DWHCI_CORE_RESET, DWHCI_CORE_RESET_AHB_IDLE, true, 100)) {
        debug("core reset busy");
        return false;
    }

    // 2. コアをソフトリセット: GRSTCTL: [0] = 1
    reset = get32(DWHCI_CORE_RESET);
    reset |= DWHCI_CORE_RESET_SOFT_RESET;
    put32(DWHCI_CORE_RESET, reset);

    // 3. ソフトリセットされるのを待つ（ GRSTCTL: [0]のbitが立つのを待つ）
    if (!dw2_hc_wait_for_bit(self, DWHCI_CORE_RESET, DWHCI_CORE_RESET_SOFT_RESET, false, 10)) {
        debug("failed soft reset");
        return false;
    }

    delayus(100*1000);

    return true;
}

// ok
void dw2_hc_enable_global_intr(dw2_hc_t *self)
{
    uint32_t ahbcfg;
    ahbcfg = get32(DWHCI_CORE_AHB_CFG);
    // 1. 割り込みを有効に
    ahbcfg |= DWHCI_CORE_AHB_CFG_GLOBALINT_MASK;
    put32(DWHCI_CORE_AHB_CFG, ahbcfg);
}

/// @brief システムレベルの割り込みをすべてクリアして、割り込みを有効にする
/// @param self USBホストコントローラ
void dw2_hc_enable_common_intr(dw2_hc_t *self)
{
    // FIXME クリアされない: put後の値が0x05000029
    uint32_t intstat;
    intstat = (uint32_t)-1;                 // 保留してる割り込みをすべてクリア
    put32(DWHCI_CORE_INT_STAT, intstat);    // 1の書き込みでクリア
}

void dw2_hc_enable_host_intr(dw2_hc_t *self)
{
    uint32_t intmask = 0;                   // すべての割り込みを無効(0)に
    put32(DWHCI_CORE_INT_MASK, intmask);

    dw2_hc_enable_common_intr(self);

    intmask = get32(DWHCI_CORE_INT_MASK);
    intmask |= (DWHCI_CORE_INT_MASK_HC_INTR
#ifdef USE_USB_SOF_INTR
            | DWHCI_CORE_INT_MASK_SOF_INTR
#endif
            );
/* if (is_plug_and_play) {
    intmask |= (DWHCI_CORE_INT_MASK_PORT_INTR | DWHCI_CORE_INT_MASK_DISCONNECT);
    }
*/
    put32(DWHCI_CORE_INT_MASK, intmask);
}

// ok
void dw2_hc_enable_channel_intr(dw2_hc_t *self, unsigned channel)
{
    uint32_t allmask;

    acquire(&self->imasklock);
    allmask = get32(DWHCI_HOST_ALLCHAN_INT_MASK);
    allmask |= (1 << channel);      // 1を書くとunmaskで割り込みが有効
    put32(DWHCI_HOST_ALLCHAN_INT_MASK, allmask);
    release(&self->imasklock);
}

// ok
void dw2_hc_disable_channel_intr(dw2_hc_t *self, unsigned channel)
{
    uint32_t allmask;

    acquire(&self->imasklock);
    allmask = get32(DWHCI_HOST_ALLCHAN_INT_MASK);
    allmask &= ~(1 << channel);     // 0を書くとmaskで割り込みが無効
    put32(DWHCI_HOST_ALLCHAN_INT_MASK, allmask);
    release(&self->imasklock);
}

void dw2_hc_flush_tx_fifo(dw2_hc_t *self, unsigned fifo)
{
    uint32_t reset = 0;
    reset |= DWHCI_CORE_RESET_TX_FIFO_FLUSH;
    reset &= ~DWHCI_CORE_RESET_TX_FIFO_NUM__MASK;
    reset |= (fifo << DWHCI_CORE_RESET_TX_FIFO_NUM__SHIFT);
    put32(DWHCI_CORE_RESET, reset);

    if (dw2_hc_wait_for_bit(self, DWHCI_CORE_RESET, DWHCI_CORE_RESET_TX_FIFO_FLUSH, false, 10)) {
        delayus(1);         // Wait for 3 PHY clocks
    }
}

void dw2_hc_flush_rx_fifo(dw2_hc_t *self)
{
    uint32_t reset = 0;
    reset |= DWHCI_CORE_RESET_RX_FIFO_FLUSH;
    put32(DWHCI_CORE_RESET, reset);

    if (dw2_hc_wait_for_bit(self, DWHCI_CORE_RESET, DWHCI_CORE_RESET_RX_FIFO_FLUSH, false, 10)) {
        delayus(1);         // Wait for 3 PHY clocks
    }
}

// ok
boolean dw2_hc_xfer_stage(dw2_hc_t *self, usb_req_t *urb, boolean in, boolean stage, unsigned timeout)
{
    trace("host=0x%p, urb=0x%p, in=%d, stage=%d, timeout=%d",
        self, urb, in, stage, timeout);
    // 1. 未使用のwaiting[]のインデックスを取得
    unsigned wblk = dw2_hc_alloc_wblock(self);

    // 1-1. waiting[]kは全て使用中
    if (wblk >= DWHCI_WAIT_BLOCKS) {
        debug("no wait block");
        return false;
    }

    // 2. リクエスト完了時のコールバック関数を設定
    usb_req_set_comp_cb(urb, dw2_hc_comp_cb, (void *)(uintptr_t)wblk, self);

    // 3. 待機中フラグを立てる(self->waiting[]はvolatile)
    self->waiting[wblk] = true;
    // 4. 非同期に転送を行う
    trace("3");
    if (!dw2_hc_xfer_stage_async(self, urb, in, stage, timeout)) {
        self->waiting[wblk] = false;
        dw2_hc_free_wblock(self, wblk);
        return false;
    }
    while (self->waiting[wblk]) {
        // dw2_hc_comp_cb()で待機中フラグが解除されるのを待つ
        yield();
    }
    dw2_hc_free_wblock(self, wblk);
    return urb->status;
}

void dw2_hc_comp_cb(usb_req_t *urb, void *param, void *ctx)
{
    //trace("called")
    // 1. 呼び出し環境を復元
    dw2_hc_t *self = (dw2_hc_t *)ctx;
    unsigned wblk = (unsigned)(uintptr_t)param;
    assert (wblk < DWHCI_WAIT_BLOCKS);
    // 2. 待機中フラグを解除
    self->waiting[wblk] = false;
}

// ok
boolean dw2_hc_xfer_stage_async(dw2_hc_t *self, usb_req_t *urb, boolean in, boolean stage, unsigned timeout)
{
    trace("host=0x%p, urb=0x%p, in=%d, stage=%d, timeout=%d",
        self, urb, in, stage, timeout);
#ifndef USE_USB_SOF_INTR
    // 1. 転送用のチャネルを割り当てる
    unsigned channel = dw2_hc_alloc_channel(self);
    trace("1: ch=%d", channel);
    // 1-1. チャネルは全て使用済み
    if (channel >= self->channels) {
        warn("all channle are used: bad channel %d", channel);
        return false;
    }
#else
    unsigned channel = DWHCI_MAX_CHANNELS;  // 使用しない
#endif

    // 2. このチャネル用のステートデータを割り当てる
    dw2_xfer_stagedata_t *stdata = (dw2_xfer_stagedata_t *)kmalloc(sizeof(dw2_xfer_stagedata_t));
    assert(stdata != 0);
    trace("2: stdata=0x%p", stdata);
    // 3. ステートデータを設定する
    dw2_xfer_stagedata(stdata, channel, urb, in, stage, timeout);
    trace("3");
#ifndef USE_USB_SOF_INTR
    // 4. このチャネルの割り込みを有効にする
    self->stdata[channel] = stdata;
    trace("enable channel interrupt: stdata[%d]=0x%p", channel, self->stdata[channel]);
    dw2_hc_enable_channel_intr(self, channel);
    trace("4");
#endif
    // 5-1. split転送ではない
    if (!stdata->split) {
        stdata->state = stage_status_no_split;
    // 5-2. split転送である（途中にハブが介在する場合のみ）
    } else {
        // 5-2-1. スプリットサイクルを開始
        // （現状は常にtrueなので以下が実行されることはない）
        if (!dw2_xfer_stagedata_begin_split_cycle(stdata)) {
#ifndef USE_USB_SOF_INTR
            dw2_hc_disable_channel_intr(self, channel);
#endif
            _dw2_xfer_stagedata(stdata);
            kmfree(stdata);
#ifndef USE_USB_SOF_INTR
            self->stdata[channel] = 0;
            dw2_hc_free_channel(self, channel);
#endif
            return false;
        }

        stdata->state = stage_status_start_split;
        stdata->split_comp = false;
        dw2_fsched_t *fsched = dw2_xfer_stagedata_get_fsched(stdata);
        assert(fsched != 0);
        fsched->start_split(fsched);
    }
#ifndef USE_USB_SOF_INTR
    // 6. トランザクション開始
    trace("6");
    dw2_hc_start_trans(self, stdata);
#else
    dw2_hc_queue_trans(self, stdata);    // 未実装でpanic
#endif

    return true;
}

#ifdef USE_USB_SOF_INTR
void dw2_hc_queue_trans(dw2_hc_t *self, dw2_xfer_stagedata_t *data)
{
    // 未実装
    assert(0);
}

void dw2_hc_queue_delay_trans(dw2_hc_t *self, dw2_xfer_stagedata_t *data)
{
    // 未実装
    assert(0);
}
#endif

// ok
void dw2_hc_start_trans(dw2_hc_t *self, dw2_xfer_stagedata_t *stdata)
{
    trace("host=0x%p, stdata=0x%p", self, stdata);
    // 1. 使用するチャネルの特定
    unsigned channel = stdata->channel;
    assert(channel < self->channels);

    // 2. チャネルは無効でなれけばならない、そうでない場合は無効にする
    uint32_t character;
    character = get32(DWHCI_HOST_CHAN_CHARACTER(channel));
    // 2-1. チャネルが有効
    if (character & DWHCI_HOST_CHAN_CHARACTER_ENABLE) {
        // 2-1-1. サブステータスをチャネル無効に
        stdata->substate = stage_substate_wait_for_channel_disaable;
        // 2-1-2. チャネルを無効にセット
        character &= ~DWHCI_HOST_CHAN_CHARACTER_ENABLE;
        // 2-1-3. チャネルを無効に（データの送受信を停止する）
        character |= DWHCI_HOST_CHAN_CHARACTER_DISABLE;
        // 2-1-4. レジスタに書き込む
        put32(DWHCI_HOST_CHAN_CHARACTER(channel), character);
        // 2-1-5. チャネル停止割り込みをマスク
        uint32_t mask = DWHCI_HOST_CHAN_INT_HALTED;
        put32(DWHCI_HOST_CHAN_INT_MASK(channel), mask);
    // 2-2. チャネルは無効
    } else {
        // 2-2-1.
        trace("2-2");;
        dw2_hc_start_channel(self, stdata);
    }
}

// ok
void dw2_hc_start_channel(dw2_hc_t *self, dw2_xfer_stagedata_t *stdata)
{
    trace("host=0x%p, stdata=0x%p", self, stdata);
    // 1. 使用するチャネルを特定
    unsigned channel = stdata->channel;
    assert(channel < self->channels);

    // 2. サブステータスを転送完了に
    stdata->substate = stage_substate_wait_for_xfer_complete;

    // 3. 保留中のチャネル割り込みをすべてクリアする
    uint32_t cintr = (uint32_t)-1;
    put32(DWHCI_HOST_CHAN_INT(channel), cintr);
    trace("3");
    // 4. 転送サイズ、パケット数、pidをセットする
    uint32_t xfersize = 0;
    // 4-1. [18;0] 転送バイト数
    xfersize |= (stdata->bpt & DWHCI_HOST_CHAN_XFER_SIZ_BYTES__MASK);
    // 4-2. [28:19]: パケット数
    xfersize |= ((stdata->ppt << DWHCI_HOST_CHAN_XFER_SIZ_PACKETS__SHIFT)
                & DWHCI_HOST_CHAN_XFER_SIZ_PACKETS__MASK);
    // 4-3. [30:29]: PID
    xfersize |= (dw2_xfer_stagedata_get_pid(stdata) << DWHCI_HOST_CHAN_XFER_SIZ_PID__SHIFT);
    // 4-4. レジスタに書き込み
    put32(DWHCI_HOST_CHAN_XFER_SIZ(channel), xfersize);
    trace("4");
    // 5. DMAアドレスをバスアドレスに変換してレジスタにセットする
    uint32_t dmaaddr;
    dmaaddr = BUS_ADDRESS(dw2_xfer_stagedata_get_dmaaddr(stdata));
    put32(DWHCI_HOST_CHAN_DMA_ADDR(channel), dmaaddr);
    trace("6: clean & invalidate");
    // 6. DMAアドレスに対応するキャッシュをクリア/無効化
    clean_and_invalidate_data_cache_range((uint64_t)stdata->buffp, (uint64_t)stdata->bpt);
    trace("6");
    // 7. データバリア
    DataMemBarrier();
    trace("7");
    // 8. スプリット転送
    // 8-1. スプリット転送を無効に
    uint32_t split = 0;
    // 8.2 スプリットをする場合はレジスタを設定
    if (stdata->split) {
        trace("8.2");;
        // 8-2-1. [6:0] ポートアドレス
        split |= dw2_xfer_stagedata_get_hubport(stdata);
        // 8-2-2. [13:7] ハブアドレス
        split |= (dw2_xfer_stagedata_get_hubaddr(stdata)
                << DWHCI_HOST_CHAN_SPLIT_CTRL_HUB_ADDRESS__SHIFT);
        // 8-2-3. [15:14] 転送位置
        split |= (dw2_xfer_stagedata_get_split_pos(stdata)
                << DWHCI_HOST_CHAN_SPLIT_CTRL_XACT_POS__SHIFT);
        // 8-2-4. [16] Do Complete Splitをホストに依頼
        if (stdata->split_comp) {
            split |= DWHCI_HOST_CHAN_SPLIT_CTRL_COMPLETE_SPLIT;
        }
        // 8-2-5. splitを有効に
        split |= DWHCI_HOST_CHAN_SPLIT_CTRL_SPLIT_ENABLE;
    }
    // 9. レジスタに書き込む
    put32(DWHCI_HOST_CHAN_SPLIT_CTRL(channel), split);
    trace("9");
    // 10. チャネルパラメタをセットする
    uint32_t character;
    character = get32(DWHCI_HOST_CHAN_CHARACTER(channel));
    // 10-1. [19:0] をクリア
    character &= ~DWHCI_HOST_CHAN_CHARACTER_MAX_PKT_SIZ__MASK;
    // 10-2. [10:0] 最大パケットサイズをセット
    character |= (stdata->xpsize & DWHCI_HOST_CHAN_CHARACTER_MAX_PKT_SIZ__MASK);
    // 10-3. [21:20] をクリア
    character &= ~DWHCI_HOST_CHAN_CHARACTER_MULTI_CNT__MASK;
    // 10-4. [21;20] = 1; 1 トランザクション / フレーム
    character |= (1 << DWHCI_HOST_CHAN_CHARACTER_MULTI_CNT__SHIFT);    // TODO: optimize
    // 10-5. [15] 方向を設定
    if (stdata->in) {
        character |= DWHCI_HOST_CHAN_CHARACTER_EP_DIRECTION_IN;
    } else {
        character &= ~DWHCI_HOST_CHAN_CHARACTER_EP_DIRECTION_IN;
    }
    // 10-6. [17] LSデバイスか
    if (stdata->speed == usb_speed_low) {
        character |= DWHCI_HOST_CHAN_CHARACTER_LOW_SPEED_DEVICE;
    } else {
        character &= ~DWHCI_HOST_CHAN_CHARACTER_LOW_SPEED_DEVICE;
    }
    // 10-7. [28:22] デバイスアドレスをクリアして設定
    character &= ~DWHCI_HOST_CHAN_CHARACTER_DEVICE_ADDRESS__MASK;
    character |= (dw2_xfer_stagedata_get_dev_addr(stdata) << DWHCI_HOST_CHAN_CHARACTER_DEVICE_ADDRESS__SHIFT);
    // 10-8. [19:18] エンドポイントタイプをクリアして設定
    character &= ~DWHCI_HOST_CHAN_CHARACTER_EP_TYPE__MASK;
    character |= (dw2_xfer_stagedata_get_ep_type(stdata) << DWHCI_HOST_CHAN_CHARACTER_EP_TYPE__SHIFT);
    // 10-9. [14:11] エンドポイント番号をクリアして設定
    character &= ~DWHCI_HOST_CHAN_CHARACTER_EP_NUMBER__MASK;
    character |= (dw2_xfer_stagedata_get_ep_number(stdata) << DWHCI_HOST_CHAN_CHARACTER_EP_NUMBER__SHIFT);
    trace("10-9: char=0x%08x", character);
    // 10-10. フレームスケジューラを取得（使用しない場合は0）
#ifndef USE_QEMU_USB_FIX
    dw2_fsched_t *fsched = dw2_xfer_stagedata_get_fsched(stdata);
    trace("10-10: fsched=0x%p", fsched);
    if (fsched != 0) {
#ifndef USE_USB_SOF_INTR
        // 10-11. フレームが送信されるのを待つ
        fsched->wait_for_frame(fsched);
        trace("10-11");;
#endif
        // 10-12. [29] 奇数フレームか
        if (fsched->is_odd_frame(fsched)) {
            // 偶数フレーム
            character |= DWHCI_HOST_CHAN_CHARACTER_PER_ODD_FRAME;
        } else {
            // 奇数フレーム
            character &= ~DWHCI_HOST_CHAN_CHARACTER_PER_ODD_FRAME;
        }
    }
#endif  // USE_QEMU_USB_FIX

    // 10-13. 割り込みマスクを設定
    uint32_t intrmask;
    intrmask = dw2_xfer_stagedata_get_status_mask(stdata);
    trace("ch=0x%x, intrmask=0x%x", channel, intrmask);
    put32(DWHCI_HOST_CHAN_INT_MASK(channel), intrmask);
    // 10-14. チャネル割り込みを有効に
    character |= DWHCI_HOST_CHAN_CHARACTER_ENABLE;
    character &= ~DWHCI_HOST_CHAN_CHARACTER_DISABLE;
    trace("char=0x%x", character);
    put32(DWHCI_HOST_CHAN_CHARACTER(channel), character);
    trace("10-13");
}

void dw2_hc_channel_intr_hdl(dw2_hc_t *self, unsigned channel)
{
    trace("called");
    dw2_xfer_stagedata_t *stdata = self->stdata[channel];
    dw2_fsched_t *fsched = dw2_xfer_stagedata_get_fsched(stdata);
    trace("stdata=0x%p, fsched=0x%p", stdata, fsched);
    usb_req_t *urb = stdata->urb;
    trace("urb=0x%p, rpenabled=0x%x", urb, self->rpenabled);
    assert(urb != 0);

    if (!self->rpenabled) {
        dw2_hc_disable_channel_intr(self, channel);
        urb->status = 0;
        urb->error = usb_err_aborted;

        _dw2_xfer_stagedata(stdata);
        kmfree(stdata);
        self->stdata[channel] = 0;

        dw2_hc_free_channel(self, channel);
        usb_req_call_comp_cb(urb);
        return;
    }

    switch(stdata->substate) {
    case stage_substate_wait_for_channel_disaable:
        trace("substate: wait_for_channel_disaable");
        dw2_hc_start_channel(self, stdata);
        return;

    case stage_substate_wait_for_xfer_complete: {
        trace("substate: wait_for_xfer_complete");
        //debug_stdata(stdata);
        clean_and_invalidate_data_cache_range((uint64_t)stdata->buffp, (uint64_t)stdata->bpt);
        //DataMemBarrier();

        uint32_t xfersize;
        xfersize = get32(DWHCI_HOST_CHAN_XFER_SIZ(channel));
        uint32_t cintr;
        cintr = get32(DWHCI_HOST_CHAN_INT(channel));
        // restart halted transaction
        if (cintr == DWHCI_HOST_CHAN_INT_HALTED) {
#ifndef USE_USB_SOF_INTR
            dw2_hc_start_trans(self, stdata);
#else
            self->stdata[channel] = 0;
            dw2_hc_free_channel(self, channel);
            dw2_hc_queue_trans(self, stdata);
#endif
            return;
        }

        assert(!dw2_xfer_stagedata_is_periodic(stdata)
             || DWHCI_HOST_CHAN_XFER_SIZ_PID(xfersize) != DWHCI_HOST_CHAN_XFER_SIZ_PID_MDATA);

        cintr = get32(DWHCI_HOST_CHAN_INT(channel));
        dw2_xfer_stagedata_trans_complete(stdata, cintr,
            DWHCI_HOST_CHAN_XFER_SIZ_PACKETS(xfersize),
            xfersize & DWHCI_HOST_CHAN_XFER_SIZ_BYTES__MASK);
        }
        break;

    default:
        assert(0);
        break;
    }

    unsigned status;
    switch(stdata->state) {
    case stage_status_no_split:
        trace("state: no_split");
        status = stdata->trstatus;
        if ((status & DWHCI_HOST_CHAN_INT_XACT_ERROR)
         && urb->ep->type == ep_type_bulk
         && dw2_xfer_stagedata_is_retry_ok(stdata)) {
#ifndef USE_USB_SOF_INTR
            dw2_hc_start_trans(self, stdata);
#else
            self->stdata[channel] = 0;
            dw2_hc_free_channel(self, channel);
            dw2_hc_queue_trans(self, stdata);
#endif
            break;
         } else if (status & DWHCI_HOST_CHAN_INT_ERROR_MASK) {
            warn("Transaction failed 1 (status 0x%x)", status);
            urb->status = 0;
            urb->error = dw2_xter_dtagedata_get_usb_err(stdata);
        } else if ((status & (DWHCI_HOST_CHAN_INT_NAK | DWHCI_HOST_CHAN_INT_NYET))
                 && dw2_xfer_stagedata_is_periodic(stdata)) {
            if (dw2_xfer_stagedata_is_timeout(stdata)) {
                urb->status = 0;
                urb->error = usb_err_timeout;
            } else {
#ifdef USE_USB_SOF_INTR
                self->stdata[channel] = 0;
                dw2_hc_free_channel(self, channel);
                dw2_hc_queue_delay_trans(stdata);
#else
                stdata->state = stage_status_periodic_delay;
                unsigned interval = urb->ep->interval;
                start_timer(MSEC2HZ(interval), dw2_hc_timer_hdlstub, stdata, self);
#endif
                break;
            }
        } else {
            if (!stdata->ststatus) {
                usb_req_set_resultlen(urb, dw2_xfer_stagedata_get_resultlen(stdata));
            }
            urb->status = 1;
        }

        dw2_hc_disable_channel_intr(self, channel);
        _dw2_xfer_stagedata(stdata);
        kmfree(stdata);
        self->stdata[channel] = 0;
        dw2_hc_free_channel(self, channel);
        usb_req_call_comp_cb(urb);
        break;

    case stage_status_start_split:
        trace("state: start_split");
        status = stdata->trstatus;
        if ((status & DWHCI_HOST_CHAN_INT_ERROR_MASK)
         || (status & DWHCI_HOST_CHAN_INT_NAK)
         || (status & DWHCI_HOST_CHAN_INT_NYET)) {
            warn("Transaction failed 2 (status 0x%x)", status);
            urb->status = 0;
            urb->error = dw2_xter_dtagedata_get_usb_err(stdata);
            dw2_hc_disable_channel_intr(self, channel);
            _dw2_xfer_stagedata(stdata);
            kmfree(stdata);
            self->stdata[channel] = 0;
            dw2_hc_free_channel(self, channel);
            usb_req_call_comp_cb(urb);
            break;
        }

        fsched->transaction_complete(fsched, status);
        stdata->state = stage_status_complete_split;
        stdata->split_comp = true;
        if (!fsched->complete_split(fsched)) {
            goto LeaveCompleteSplit;
        }
#ifndef USE_USB_SOF_INTR
        dw2_hc_start_trans(self, stdata);
#else
        self->stdata[channel] = 0;
        dw2_hc_free_channel(self, channel);
        dw2_hc_queue_trans(self, stdata);
#endif
        break;

    case stage_status_complete_split:
        trace("state: complete_split");
        status = stdata->trstatus;
        if (status & DWHCI_HOST_CHAN_INT_ERROR_MASK) {
            warn("Transaction failed 3 (status 0x%x)", status);
            urb->status = 0;
            urb->error = dw2_xter_dtagedata_get_usb_err(stdata);
            dw2_hc_disable_channel_intr(self, channel);
            _dw2_xfer_stagedata(stdata);
            kmfree(stdata);
            self->stdata[channel] = 0;
            dw2_hc_free_channel(self, channel);
            usb_req_call_comp_cb(urb);
            break;
        }

        fsched->transaction_complete(fsched, status);
        if (fsched->complete_split(fsched)) {
#ifndef USE_USB_SOF_INTR
            dw2_hc_start_trans(self, stdata);
#else
            self->stdata[channel] = 0;
            dw2_hc_free_channel(self, channel);
            dw2_hc_queue_trans(self, stdata);
#endif
            break;
        }
    LeaveCompleteSplit:
        if (!dw2_xfer_stagedata_is_stage_complete(stdata)) {
            if (!dw2_xfer_stagedata_begin_split_cycle(stdata)) {
                urb->status = 0;
                urb->error = usb_err_split;
                dw2_hc_disable_channel_intr(self, channel);
                _dw2_xfer_stagedata(stdata);
                kmfree(stdata);
                self->stdata[channel] = 0;
                dw2_hc_free_channel(self, channel);
                usb_req_call_comp_cb(urb);
                break;
            }

            if (!dw2_xfer_stagedata_is_periodic(stdata)) {
                stdata->state = stage_status_start_split;
                stdata->split_comp = false;
                fsched->start_split(fsched);
#ifndef USE_USB_SOF_INTR
                dw2_hc_start_trans(self, stdata);
#else
                self->stdata[channel] = 0;
                dw2_hc_free_channel(self, channel);
                dw2_hc_queue_trans(self, stdata);
#endif
            } else {
                if (dw2_xfer_stagedata_is_timeout(stdata)) {
                    dw2_hc_disable_channel_intr(self, channel);
                    urb->status = 0;
                    urb->error = usb_err_timeout;
                    _dw2_xfer_stagedata(stdata);
                    kmfree(stdata);
                    self->stdata[channel] = 0;
                    dw2_hc_free_channel(self, channel);
                    usb_req_call_comp_cb(urb);
                } else {
#ifdef USE_USB_SOF_INTR
                    self->stdata[channel] = 0;
                    dw2_hc_free_channel(self, channel);
                    dw2_hc_queue_delay_trans(self, stdata);
#else
                    stdata->state = stage_status_periodic_delay;
                    unsigned interval = urb->ep->interval;
                    start_timer(MSEC2HZ(interval), dw2_hc_timer_hdlstub, stdata, self);
#endif
                }
            }
            break;
        }

        dw2_hc_disable_channel_intr(self, channel);
        if (!stdata->ststatus) {
            urb->resultlen = dw2_xfer_stagedata_get_resultlen(stdata);
        }
        urb->status = 1;
        _dw2_xfer_stagedata(stdata);
        kmfree(stdata);
        self->stdata[channel] = 0;
        dw2_hc_free_channel(self, channel);
        usb_req_call_comp_cb(urb);
        break;

    default:
        assert(0);
        break;
    }
}

#ifdef USE_USB_SOF_INTR

void dw2_hc_sof_intr_hdl(dw2_hc_t * self)
{
    // 未実装
    assert(0);

}

#endif


void dw2_hc_intr_hdl(dw2_hc_t *self)
{
    DataMemBarrier();

    uint32_t intstat;
    intstat = get32(DWHCI_CORE_INT_STAT);

#ifdef USE_USB_SOF_INTR
    if (intstat & DWHCI_CORE_INT_STAT_SOF_INTR) {
        dw2_hc_sof_intr_hdl(self);
    }
#endif

    if (intstat & DWHCI_CORE_INT_STAT_HC_INTR) {
        uint32_t allchanintr;
        allchanintr = get32(DWHCI_HOST_ALLCHAN_INT);
        put32(DWHCI_HOST_ALLCHAN_INT, allchanintr);

        uint32_t cmask = 1;
        for (unsigned channel = 0; channel < self->channels; channel++) {
            allchanintr = get32(DWHCI_HOST_ALLCHAN_INT);
            if (allchanintr & cmask) {
                uint32_t intrmask = 0;
                put32(DWHCI_HOST_CHAN_INT_MASK(channel), intrmask);
                dw2_hc_channel_intr_hdl(self, channel);
            }
            cmask <<= 1;
        }
    }

/* plug and play関連
    if (self->pap) {
        if (intstat & DWHCI_CORE_INT_STAT_PORT_INTR) {
            uint32_t hostport;
            hostport = get32(DWHCI_HOST_PORT);
            debug("Port interrupt(status 0x%x)", hostport);

            hostport &= ~DWHCI_HOST_PORT_ENABLE;
            hostport |= (DWHCI_HOST_PORT_CONNECT_CHANGED
                    | DWHCI_HOST_PORT_ENABLE_CHANGED
                    | DWHCI_HOST_PORT_OVERCURRENT_CHANGED);
            put32(hostport, DWHCI_HOST_PORT);

            if (intstat & DWHCI_CORE_INT_MASK_DISCONNECT) {
                dw2_rport_port_status_changed(&self->rport);
            }

            hostport &= ~DWHCI_HOST_PORT_ENABLE;
            hostport |= (DWHCI_HOST_PORT_CONNECT_CHANGED
                       | DWHCI_HOST_PORT_ENABLE_CHANGED
                       | DWHCI_HOST_PORT_OVERCURRENT_CHANGED);
            put32(DWHCI_HOST_PORT, hostport);
        }

        if (intstat & DWHCI_CORE_INT_MASK_DISCONNECT) {
            dw2_rport_port_status_changed(&self->rport);
        }
    }
*/

    put32(DWHCI_CORE_INT_STAT, intstat);

    DataMemBarrier();
}

void dw2_hc_intr_hdlstub(void *param)
{
    dw2_hc_t *self = (dw2_hc_t *) param;
    assert(self != 0);
    dw2_hc_intr_hdl(self);
}

#ifndef USE_USB_SOF_INTR

void dw2_hc_timer_hdl(dw2_hc_t *self, dw2_xfer_stagedata_t *stdata)
{
    DataMemBarrier();

    assert(stdata != 0);

    if (!self->rpenabled) {
        unsigned channel = stdata->channel;
        assert(channel < self->channels);
        usb_req_t *urb = stdata->urb;
        assert(urb != 0);

        dw2_hc_disable_channel_intr(self, channel);

        urb->status = 0;
        urb->error = usb_err_aborted;

        _dw2_xfer_stagedata(stdata);
        kmfree(stdata);
        self->stdata[channel] = 0;
        dw2_hc_free_channel(self, channel);

        DataMemBarrier();

        usb_req_call_comp_cb(urb);

        return;
    }
    trace("st=%d", stdata->state);
    assert(stdata->state == stage_status_periodic_delay);

    if (stdata->split) {
        stdata->state = stage_status_start_split;
        stdata->split_comp = false;

        dw2_fsched_t *fsched = dw2_xfer_stagedata_get_fsched(stdata);
        assert(fsched != 0);
        fsched->start_split(fsched);
    } else {
        stdata->state = stage_status_no_split;
    }

    dw2_hc_start_trans(self, stdata);

    DataMemBarrier();
}

void dw2_hc_timer_hdlstub(uint64_t data)
{
    uint64_t *params = (uint64_t *)data;
    dw2_hc_t *self = (dw2_hc_t *)params[0];
    assert(self != 0);
    dw2_xfer_stagedata_t *stdata = (dw2_xfer_stagedata_t *)params[1];
    assert(stdata != 0);

    dw2_hc_timer_hdl(self, stdata);

    // FIXME: これはいる?
    kmfree(&data);
}

#endif

// ok
unsigned dw2_hc_alloc_channel(dw2_hc_t *self)
{
    assert(self != 0);

    acquire(&self->chanlock);
    // 1. 未使用のチャネル番号を探す
    unsigned cmask = 1;
    for (unsigned channel = 0; channel < self->channels; channel++) {
        // 2. channelが未使用
        if (!(self->allocch & cmask)) {
            // 3. 使用済みフラグを立てる
            self->allocch |= cmask;
            release(&self->chanlock);
            // 4. channelをチャネル番号とする
            return channel;
        }

        cmask <<= 1;
    }
    // 5. チャネルはすべて使用済み
    release(&self->chanlock);
    return DWHCI_MAX_CHANNELS;
}

void dw2_hc_free_channel(dw2_hc_t *self, unsigned channel)
{
    assert(self != 0);
    assert(channel < self->channels);

    unsigned cmask = 1 << channel;
    acquire(&self->chanlock);
    //info("allocch=0x%x", self->allocch);
    //assert(self->allocch & cmask);
    self->allocch &= ~cmask;
    release(&self->chanlock);
}

// ok
unsigned dw2_hc_alloc_wblock(dw2_hc_t *self)
{
    acquire(&self->wblklock);
    unsigned mask = 1;
    for (unsigned blk = 0; blk < DWHCI_WAIT_BLOCKS; blk++) {
        // 1. waiting[blk]が未使用
        if (!(self->allocwblk & mask)) {
            // 2. 使用済みとする
            self->allocwblk |= mask;
            release(&self->wblklock);
            // 3. 使用する配列のインデックスを返す
            return blk;
        }
        mask <<= 1;
    }
    release(&self->wblklock);
    return DWHCI_WAIT_BLOCKS;
}

void dw2_hc_free_wblock(dw2_hc_t *self, unsigned wblock)
{
    assert(wblock < DWHCI_WAIT_BLOCKS);

    unsigned mask = 1 << wblock;
    acquire(&self->wblklock);
    assert(self->allocwblk & mask);
    self->allocwblk &= ~mask;
    release(&self->wblklock);
}

// ok
boolean dw2_hc_wait_for_bit(dw2_hc_t *self, uint64_t reg, uint32_t mask, boolean bit, unsigned timeout)
{
    // 対象ビットが立ったらwhileを抜ける
    while((get32(reg) & mask) ? !bit : bit) {
        delayus(1000);
        if (--timeout == 0) {
            return false;
        }
    }

    return true;
}

// ok
void dw2_hc_dumreg(dw2_hc_t *self, const char *name, uint64_t addr)
{
    DataMemBarrier();
    debug("0x%08x: %s", get32(addr), name);
}

// ok
void dw2_hc_dump_status(dw2_hc_t *self, unsigned channel)
{
    dw2_hc_dumreg(self, "OTG_CTRL",     DWHCI_CORE_OTG_CTRL);
    dw2_hc_dumreg(self, "AHB_CFG",      DWHCI_CORE_AHB_CFG);
    dw2_hc_dumreg(self, "USB_CFG",      DWHCI_CORE_USB_CFG);
    dw2_hc_dumreg(self, "RESET",        DWHCI_CORE_RESET);
    dw2_hc_dumreg(self, "INT_STAT",     DWHCI_CORE_INT_STAT);
    dw2_hc_dumreg(self, "INT_MASK",     DWHCI_CORE_INT_MASK);
    dw2_hc_dumreg(self, "RX_FIFO_SIZ",  DWHCI_CORE_RX_FIFO_SIZ);
    dw2_hc_dumreg(self, "NPER_TX_FIFO_SIZ",     DWHCI_CORE_NPER_TX_FIFO_SIZ);
    dw2_hc_dumreg(self, "NPER_TX_STAT",         DWHCI_CORE_NPER_TX_STAT);
    dw2_hc_dumreg(self, "HOST_PER_TX_FIFO_SIZ", DWHCI_CORE_HOST_PER_TX_FIFO_SIZ);

    dw2_hc_dumreg(self, "HOST_CFG",             DWHCI_HOST_CFG);
    dw2_hc_dumreg(self, "HOST_PER_TX_FIFO_STAT",    DWHCI_HOST_PER_TX_FIFO_STAT);
    dw2_hc_dumreg(self, "HOST_ALLCHAN_INT",     DWHCI_HOST_ALLCHAN_INT);
    dw2_hc_dumreg(self, "HOST_ALLCHAN_INT_MASK",    DWHCI_HOST_ALLCHAN_INT_MASK);
    dw2_hc_dumreg(self, "HOST_PORT",            DWHCI_HOST_PORT);

    dw2_hc_dumreg(self, "HOST_CHAN_CHARACTER(n)",   DWHCI_HOST_CHAN_CHARACTER(channel));
    dw2_hc_dumreg(self, "HOST_CHAN_SPLIT_CTRL(n)",  DWHCI_HOST_CHAN_SPLIT_CTRL(channel));
    dw2_hc_dumreg(self, "HOST_CHAN_INT(n)",         DWHCI_HOST_CHAN_INT(channel));
    dw2_hc_dumreg(self, "HOST_CHAN_INT_MASK(n)",    DWHCI_HOST_CHAN_INT_MASK(channel));
    dw2_hc_dumreg(self, "HOST_CHAN_XFER_SIZ(n)",    DWHCI_HOST_CHAN_XFER_SIZ(channel));
    dw2_hc_dumreg(self, "HOST_CHAN_DMA_ADDR(n)",    DWHCI_HOST_CHAN_DMA_ADDR(channel));
}

// ここからは USB Host Controllerの関数

// ok
int dw2_hc_get_desc(dw2_hc_t *self, usb_ep_t *ep,
                  unsigned char type, unsigned char index,
                  void *buffer, unsigned buflen,
                  unsigned char reqtype, unsigned short idx)
{
    // コントロール転送を行う
    trace("call dw2_hc_control_message");
    return dw2_hc_control_message(self, ep,
                    reqtype, GET_DESCRIPTOR,
                   (type << 8) | index, idx,
                    buffer, buflen);
}

// ok
boolean dw2_hc_set_addr(dw2_hc_t *self, usb_ep_t *ep, uint8_t devaddr)
{
    if (dw2_hc_control_message(self, ep, REQUEST_OUT, SET_ADDRESS, devaddr, 0, 0, 0) < 0) {
        debug("failed to set dev addr");
        return false;
    }
    // 50 ms待つ
    delayus(50*1000);        // see USB 2.0 spec(tDSETADDR): 表7-14
    return true;
}

// ok
boolean dw2_hc_set_config(dw2_hc_t *self, usb_ep_t *ep, uint8_t cfgvalue)
{
    if (dw2_hc_control_message(self, ep, REQUEST_OUT, SET_CONFIGURATION, cfgvalue, 0, 0, 0) < 0) {
        debug("failed to set config value");
        return false;
    }
    delayus(50*1000);
    return true;
}

// ok
int dw2_hc_control_message(dw2_hc_t *self, usb_ep_t *ep,
            uint8_t reqtype, uint8_t req, uint16_t value, uint16_t index,
            void *data, uint16_t datalen)
{
/*
    info("host=0x%p, ep=0x%p, type=0x%x, req=%d, value=0x%04x, index=%d, len=%d",
        self, ep, reqtype, req, value, index, datalen);
    if (datalen > 0) {
        cprintf("  data: 0x%p (0x%x bytes)\n   ", data, datalen);
        for (int i=0; i<datalen; i++) {
            cprintf(" %02x", *(((char *)data) + i));
        }
        cprintf("\n\n");
    }
*/
    // 1. SetUpパケット用のDMAバッファを用意
    setup_data_t setupdata GALIGN(4);        // DMA buffer
    trace("setup_data: 0x%p", &setupdata);
    // 2. SetUpパケットにデータをセット
    setupdata.reqtype   = reqtype;
    setupdata.req       = req;
    setupdata.value     = value;
    setupdata.index     = index;
    setupdata.length    = datalen;

    // 3. USBリクエストを作成
    usb_req_t urb;
    usb_request(&urb, ep, data, datalen, &setupdata);
    trace("3");
    int result = -1;
    // 4. リクエストを送信
    if (dw2_hc_submit_block_request(self, &urb, USB_TIMEOUT_NONE)) {
        result = urb.resultlen;
    }
    // 4. USBリクエストを破棄
    _usb_request(&urb);
    trace("result len=%d", result);
    return result;
}

int dw2_hc_xfer(dw2_hc_t *self, usb_ep_t *ep, void *buffer, unsigned buflen)
{
    usb_req_t urb;
    usb_request(&urb, ep, buffer, buflen, 0);

    int result = -1;
    if (dw2_hc_submit_block_request(self, &urb, USB_TIMEOUT_NONE)) {
        result = urb.resultlen;
    }
    _usb_request(&urb);

    return result;
}

/* plug and play関連
boolean dw2_hc_is_pap(dw2_hc_t *self)
{
    return self->pap;
}

boolean dw2_hc_update_pap(dw2_hc_t *self)
{
    boolean result = true;

    acquire(&self->hublock);
    pst_ev_t *ev, *nev;
    LIST_FOREACH_ENTRY_SAFE(ev, nev, &self->hublist, list) {
        list_drop(ev);
        release(&self->hublock);
        if (ev->rport)
            dw2_rport_handle_port_status_change(ev->rport);
        else
            usb_stdhub_handle_port_status_change(ev->hub);
        kmfree(ev);
        result = true;
        acquire(&self->hublock);
    }
    release(&self->hublock);

    return result;
}
*/

void usbhc_init(void)
{
    if (dw2hc != 0)
        return;

    dev_ns_t *dev_ns = (dev_ns_t *)kmalloc(sizeof(dev_ns_t));
    dev_name_service(dev_ns);
    dw2hc = (dw2_hc_t *)kmalloc(sizeof(dw2_hc_t));
    dw2_hc(dw2hc);
    if (dw2_hc_init(dw2hc, true)) {
        info("dw2hc initialized\n");
    } else {
        panic("failed to initialize dw2_hc\n");
    }

}

void usb_test(void)
{
    info("usb_test start/end\n");
}
