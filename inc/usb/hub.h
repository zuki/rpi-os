//
// usbhub.h
//
// Definitions for USB hubs
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef INC_USB_HUB_H
#define INC_USB_HUB_H

#include "types.h"

// 構成
#define USB_HUB_MAX_PORTS       8        // TODO

// デバイスクラス
#define USB_DEVICE_CLASS_HUB    9

// クラス固有リクエスト
#define RESET_TT                9

// ディスクリプタ種別
#define DESCRIPTOR_HUB          0x29

// 機能セレクタ
#define PORT_ENABLE             1
#define PORT_RESET              4
#define PORT_POWER              8
#define C_PORT_CONNECTION       16
#define C_PORT_ENABLE           17
#define C_PORT_SUSPEND          18
#define C_PORT_OVER_CURRENT     19
#define C_PORT_RESET            20

// ハブディスクリプタ構造体
typedef struct usb_hub_descriptor {
    unsigned char   length;
    unsigned char   type;
    unsigned char   nports;
    unsigned short  hubchars;
        #define HUB_POWER_MODE(reg)         ((reg) & 3)
            #define HUB_POWER_MODE_GANGED       0
            #define HUB_POWER_MODE_INDIVIDUAL   1
        #define HUB_TT_THINK_TIME(reg)      (((reg) >> 5) & 3)
    unsigned char   pwrgood;
    unsigned char   current;
    unsigned char   removeable[1];          // max. 8 ports
    unsigned char   mask[1];                // max. 8 ports
} PACKED hub_desc_t;

typedef struct usb_hub_status {
    unsigned short  status;
        #define HUB_LOCAL_POWER_LOST__MASK  (1 << 0)
        #define HUB_OVER_CURRENT__MASK      (1 << 1)
    unsigned short  change;
        #define C_HUB_LOCAL_POWER_LOST__MASK (1 << 0)
        #define C_HUB_OVER_CURRENT__MASK    (1 << 1)
} PACKED hub_status_t;

typedef struct usb_port_status {
    unsigned short  status;
        #define PORT_CONNECTION__MASK       (1 << 0)
        #define PORT_ENABLE__MASK           (1 << 1)
        #define PORT_OVER_CURRENT__MASK     (1 << 3)
        #define PORT_RESET__MASK            (1 << 4)
        #define PORT_POWER__MASK            (1 << 8)
        #define PORT_LOW_SPEED__MASK        (1 << 9)
        #define PORT_HIGH_SPEED__MASK       (1 << 10)
    unsigned short  change;
        #define C_PORT_CONNECTION__MASK     (1 << 0)
        #define C_PORT_ENABLE__MASK         (1 << 1)
        #define C_PORT_SUSPEND__MASK        (1 << 2)
        #define C_PORT_OVER_CURRENT__MASK   (1 << 3)
        #define C_PORT_RESET__MASK          (1 << 4)
} PACKED usb_port_status_t;

#endif
