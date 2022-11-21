//
// usbendpoint.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright(C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef INC_USB_ENDPOINT_H
#define INC_USB_ENDPOINT_H

#include "usb/usb.h"
#include "usb/device.h"
#include "types.h"

/// @brief エンドポイント転送種別
typedef enum {
    ep_type_control,
    ep_type_bulk,
    ep_type_interrupt,
    ep_type_isochronous
} ep_type_t;

/// @brief USBエンドポイント構造体

typedef struct usb_ep {
    struct usb_dev *dev;                ///< デバイス
    uint8_t         num;                ///< エンドポイント番号
    ep_type_t       type;               ///< 種別
    boolean         in;                 ///< 転送方向
    uint32_t        xsize;              ///< 最大パケットサイズ
    unsigned        interval;           ///< 間隔（ミリ秒）
    usb_pid_t       nextpid;            ///< 次のPID
} usb_ep_t;

void usb_endpoint(usb_ep_t *self, struct usb_dev *dev);  // for ep0
void usb_endpoint2(usb_ep_t *self, struct usb_dev *dev, const ep_desc_t *desc);
void usb_endpoint_copy(usb_ep_t *self, usb_ep_t *ep, struct usb_dev *dev);
void _usb_endpoint(usb_ep_t *self);

// getter
struct usb_dev *usb_ep_get_device(usb_ep_t *self);
uint8_t usb_ep_get_number(usb_ep_t *self);
ep_type_t usb_ep_get_type(usb_ep_t *self);
uint32_t usb_ep_get_max_packet_size(usb_ep_t *self);
unsigned usb_ep_get_interval(usb_ep_t *self);		// Milliseconds
// setter
void usb_ep_set_max_packet_size(usb_ep_t *self, uint32_t xsize);
// その他
boolean usb_ep_is_direction_in(usb_ep_t *self);
usb_pid_t usb_ep_get_nextpid(usb_ep_t *self, boolean ststage);
void usb_ep_skip_pid(usb_ep_t *self, unsigned packets, boolean ststage);
void usb_ep_reset_pid(usb_ep_t *self);

#endif
