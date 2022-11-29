//
// dwhcixferstagedata.h
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
#ifndef INC_USB_DW2XFERSTAGEDATA_H
#define INC_USB_DW2XFERSTAGEDATA_H

#include "usb/usb.h"
#include "usb/request.h"
#include "usb/device.h"
#include "usb/endpoint.h"
#include "usb/dw2fscheduler.h"
#include "usb/dw2fschedper.h"
#include "usb/dw2fschednper.h"
#include "usb/dw2fschednsplit.h"
#include "types.h"

#define USB_TIMEOUT_NONE    0    // Wait forever

typedef struct dw2_xfer_stagedata {
    unsigned        channel;            ///< チャネル
    usb_req_t      *urb;                ///< リクエスト
    boolean         in;                 ///< 方向（INか）
    boolean         ststatus;           ///< ステージステータス

    boolean         split;              ///< 分割トランザクションか
    boolean         split_comp;         ///< 分割完了か
    unsigned        timeout;            ///< タイムアウト

    usb_dev_t      *dev;                ///< デバイス
    usb_ep_t       *ep;                 ///< エンドポイント
    usb_speed_t     speed;              ///< 速度
    uint32_t        xpsize;             ///< 最大パケットサイズ

    uint32_t        xfersize;           ///< 転送サイズ
    unsigned        packets;            ///< パケット数
    uint32_t        bpt;                ///< トランザクションあたりのバイト数
    unsigned        ppt;                ///< トランザクションあたりのパケット数
    uint32_t        xfered;             ///< 転送済み層バイト数

    unsigned        state;              ///< 状態
    unsigned        substate;           ///< 副状態
    uint32_t        trstatus;           ///< トランザクション状態
    unsigned        err_cnt;            ///< エラー数

    uint32_t        buffer[16] GALIGN(4);   ///< DMA buffer
    void           *buffp;              ///< バッファへのポインタ

    unsigned        start;              ///< スタート時(tickHZ)

    boolean         fsused;
    union {
        dw2_fsched_t         base;
        dw2_fsched_per_t     periodic;
        dw2_fsched_nper_t    nonperiodic;
        dw2_fsched_nsplit_t  nosplit;
    } fsched;                           ///< フレームスケジューラ
} dw2_xfer_stagedata_t;

void dw2_xfer_stagedata(dw2_xfer_stagedata_t *self, unsigned channel, usb_req_t *urb, boolean in, boolean ststatus, unsigned timeout);

void _dw2_xfer_stagedata(dw2_xfer_stagedata_t *self);

// change status
void dw2_xfer_stagedata_trans_complete(dw2_xfer_stagedata_t *self, uint32_t status, uint32_t packetleft, uint32_t byteleft);
void dw2_xfer_stagedata_set_split_complete(dw2_xfer_stagedata_t *self, boolean comp);

void dw2_xfer_stagedata_set_state(dw2_xfer_stagedata_t *self, unsigned state);
unsigned dw2_xfer_stagedata_get_state(dw2_xfer_stagedata_t *self);
void dw2_xfer_stagedata_set_sub_state(dw2_xfer_stagedata_t *self, unsigned substate);
unsigned dw2_xfer_stagedata_get_sub_state(dw2_xfer_stagedata_t *self);

boolean dw2_xfer_stagedata_begin_split_cycle(dw2_xfer_stagedata_t *self);

// get transaction parameters
unsigned dw2_xfer_stagedata_get_channel_number(dw2_xfer_stagedata_t *self);
uint8_t dw2_xfer_stagedata_get_dev_addr(dw2_xfer_stagedata_t *self);
boolean dw2_xfer_stagedata_is_periodic(dw2_xfer_stagedata_t *self);
uint8_t dw2_xfer_stagedata_get_ep_type(dw2_xfer_stagedata_t *self);
uint8_t dw2_xfer_stagedata_get_ep_number(dw2_xfer_stagedata_t *self);
uint32_t dw2_xfer_stagedata_get_max_packet_size(dw2_xfer_stagedata_t *self);
usb_speed_t dw2_xfer_stagedata_get_speed(dw2_xfer_stagedata_t *self);

uint8_t dw2_xfer_stagedata_get_pid(dw2_xfer_stagedata_t *self);
boolean dw2_xfer_stagedata_is_in(dw2_xfer_stagedata_t *self);
boolean dw2_xfer_stagedata_is_ststage(dw2_xfer_stagedata_t *self);

uint64_t dw2_xfer_stagedata_get_dmaaddr(dw2_xfer_stagedata_t *self);
uint32_t dw2_xfer_stagedata_get_byte2xfer(dw2_xfer_stagedata_t *self);
uint32_t dw2_xfer_stagedata_get_packet2xfer(dw2_xfer_stagedata_t *self);

boolean dw2_xfer_stagedata_is_split(dw2_xfer_stagedata_t *self);
boolean dw2_xfer_stagedata_is_split_complete(dw2_xfer_stagedata_t *self);
uint8_t dw2_xfer_stagedata_get_hubaddr(dw2_xfer_stagedata_t *self);
uint8_t dw2_xfer_stagedata_get_hubport(dw2_xfer_stagedata_t *self);
uint8_t dw2_xfer_stagedata_get_split_pos(dw2_xfer_stagedata_t *self);

uint32_t dw2_xfer_stagedata_get_status_mask(dw2_xfer_stagedata_t *self);

// check status after transaction
uint32_t dw2_xfer_stagedata_get_trstatus(dw2_xfer_stagedata_t *self);
boolean dw2_xfer_stagedata_is_stage_complete(dw2_xfer_stagedata_t *self);
uint32_t dw2_xfer_stagedata_get_resultlen(dw2_xfer_stagedata_t *self);

usb_req_t *dw2_xfer_stagedata_get_urb(dw2_xfer_stagedata_t *self);
dw2_fsched_t *dw2_xfer_stagedata_get_fsched(dw2_xfer_stagedata_t *self);

boolean dw2_xfer_stagedata_is_retry_ok(dw2_xfer_stagedata_t *self);

usb_err_t dw2_xter_dtagedata_get_usb_err(dw2_xfer_stagedata_t *self);

boolean dw2_xfer_stagedata_is_timeout(dw2_xfer_stagedata_t *self);

void debug_stdata(dw2_xfer_stagedata_t *self);

#endif
