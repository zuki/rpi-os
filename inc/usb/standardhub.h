//
// usbstandardhub.h
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef INC_USB_STANDARDHUB_H
#define INC_USB_STANDARDHUB_H

#include "usb/usb.h"
#include "usb/device.h"
#include "usb/hub.h"
#include "usb/function.h"
#include "usb/request.h"
#include "types.h"
#include "string.h"

struct usb_ep;

typedef struct usb_standardhub {
	usb_func_t          func;       ///< usb functionオブジェクト
    struct usb_ep      *intr_ep;    ///< 割り込みエンドポイント
    hub_desc_t         *hub_desc;   ///< ハブディスクリプタ
    uint8_t            *buffer;     ///< ステータス変更データ用のバッファ
    unsigned            nports;     ///< ポート数
    boolean             poweron;    ///< 電源オンか
    uint32_t            devno;      ///< デバイス番号("uhubXX"のXX部)
    usb_dev_t     *devs[USB_HUB_MAX_PORTS];    ///< ポートデバイス配列
    usb_port_status_t  *status[USB_HUB_MAX_PORTS];  ///< ポートステータス配列
    boolean             portconf[USB_HUB_MAX_PORTS]; ///< ポート構成済み配列
} usb_stdhub_t;

void usb_standardhub(usb_stdhub_t *self, usb_func_t *func);
void _usb_standardhub(usb_stdhub_t *self);
boolean usb_stdhub_init(usb_stdhub_t *self);
boolean usb_stdhub_config(usb_func_t *self);
boolean usb_stdhub_rescan_dev(usb_stdhub_t *self);
boolean usb_stdhub_remove_dev(usb_stdhub_t *self, uint32_t index);
boolean usb_stdhub_disable_port(usb_stdhub_t *self, uint32_t index);
void usb_stdhub_handle_port_status_change(usb_stdhub_t *self);

boolean usb_stdhub_start_status_change_req(usb_stdhub_t *self);
void usb_stdhub_comp_cb(usb_stdhub_t *self, usb_req_t *urb);
void usb_stdhub_comp_cbstub(usb_req_t *urb, void *param, void *ctx);

void usb_stdhub_port_status_changed(usb_stdhub_t *self);

#endif
