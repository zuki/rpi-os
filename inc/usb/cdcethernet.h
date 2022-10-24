//
// usbcdcethernet.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2019  R. Stange <rsta2@o2online.de>
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
#ifndef INC_USB_CDCETHERNET_H
#define INC_USB_CDCETHERNET_H

#include "types.h"
#include "usb/usb.h"
#include "usb/device.h"
#include "usb/endpoint.h"
#include "usb/function.h"
#include "netdevice.h"


/// @brief CDC Ethernetデバイス構造体
typedef struct cdcethernet {
    usb_func_t   usb_func;
    net_dev_t   *net_dev;
    usb_ep_t    *bulk_in;       ///< バルク転送入力用パイプ
    usb_ep_t    *bulk_out;      ///< バルク転送出力用パイプ
    char         macaddr[MAC_ADDRESS_SIZE]; /// MACアドレス
} cdcether_t;

void cdcether(cdcether_t *self, usb_func_t *func);
void _cdcether(cdcether_t *self);

boolean cdcether_configure(usb_func_t *func);
boolean cdcether_send_frame(cdcether_t *self, const void *buffer, uint32_t len);
boolean cdcether_receive_frame(cdcether_t *self, void *buff, uint32_t *resultlen);
boolean cdcether_init_macaddr(cdcether_t *self, uint8_t id);

const char *cdcether_get_macaddr(cdcether_t *self);

#endif
