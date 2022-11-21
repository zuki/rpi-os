//
// usbcdcethernet.cpp
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
#include "usb/cdcethernet.h"
#include "usb/usb.h"
#include "usb/request.h"
#include "usb/string.h"
#include "usb/dw2hcd.h"
#include "usb/devicenameservice.h"
#include "types.h"
#include "console.h"
#include "kmalloc.h"
#include "mbox.h"
#include "clock.h"
#include "arm.h"
//#include "netdevice.h"
#include "string.h"

typedef struct ethernet_functional_descriptor {
    uint8_t      length;
    uint8_t      type;
    uint8_t      subtype;
#define ETHERNET_NETWORKING_FUNCTIONAL_DESCRIPTOR    0x0F
    uint8_t      macaddr;
    uint32_t     statics;
    uint16_t     xsegsize;
    uint16_t     mcfilters;
    uint8_t      powerfilters;
} PACKED eth_func_desc_t;

void cdcether(cdcether_t *self, usb_func_t *func)
{
    usb_func_copy(&self->usb_func, func);
    //self->net_dev = net_dev();
    self->usb_func.configure = cdcether_configure;
    self->bulk_in = 0;
    self->bulk_out = 0;
}

void _cdcether(cdcether_t *self)
{
    if (self->bulk_out != 0) {
        _usb_endpoint(self->bulk_out);
        kmfree(self->bulk_out);
    }

    if (self->bulk_in != 0) {
        _usb_endpoint(self->bulk_in);
        kmfree(self->bulk_in);
    }

    _usb_function(&self->usb_func);
}


boolean cdcether_configure(usb_func_t *func)
{
    cdcether_t *self = (cdcether_t *)func;

    // Ethernetネットワーキング機能ディスクリプタを見つける
    const eth_func_desc_t *eth_desc;
    while ((eth_desc = (eth_func_desc_t *)usb_func_get_desc(&self->usb_func, DESCRIPTOR_CS_INTERFACE)) != 0) {
        if (eth_desc->subtype == ETHERNET_NETWORKING_FUNCTIONAL_DESCRIPTOR)
            break;
    }

    if (eth_desc == 0) {
        error("couldn't find ETHERNET_NETWORKING_FUNCTIONAL_DESCRIPTOR");
        return false;
    }

    // データクラスインタフェースディスクリプタを見つける
    const if_desc_t *if_decs;
    while ((if_decs = (if_desc_t *)usb_func_get_desc(&self->usb_func, DESCRIPTOR_INTERFACE)) != 0) {
        if (if_decs->class    == 0x0A
         && if_decs->subclass == 0x00
         && if_decs->proto    == 0x00
         && if_decs->neps     >= 2) {
            break;
        }
    }

    if (if_decs == 0) {
        error("couldn't find DESCRIPTOR_INTERFACE");
        return false;
    }

    // MACアドレスを初期化
    if (!cdcether_init_macaddr(self, eth_desc->macaddr)) {
        error("Cannot get MAC address");
        return false;
    }

    info("MAC address is %x:%x:%x:%x:%x:%x", self->macaddr[0], self->macaddr[1], self->macaddr[2], self->macaddr[3], self->macaddr[4], self->macaddr[5]);

    // エンドポイントを取得
    const ep_desc_t *ep_desc;
    while ((ep_desc = (ep_desc_t *)usb_func_get_desc(&self->usb_func, DESCRIPTOR_ENDPOINT)) != 0) {
        if ((ep_desc->attr & 0x3F) == 0x02) {       // バルク転送
            if ((ep_desc->addr & 0x80) == 0x80) {   // 入力パイプ
                if (self->bulk_in != 0) {
                    error("bulk_in not null");
                    return false;
                }
                self->bulk_in = (usb_ep_t *)kmalloc(sizeof(usb_ep_t));
                usb_endpoint2(self->bulk_in, usb_func_get_dev(&self->usb_func), ep_desc);
            } else {                                // 出力パイプ
                if (self->bulk_out != 0) {
                    error("bulk_out not null");
                    return false;
                }
                self->bulk_out = (usb_ep_t *)kmalloc(sizeof(usb_ep_t));
                usb_endpoint2(self->bulk_out, usb_func_get_dev(&self->usb_func), ep_desc);
            }
        }
    }

    if (self->bulk_in == 0 || self->bulk_out == 0) {
        error("ep bulk_in and bulk_out must exit");
        return false;
    }

    // コンフィグレーションを行う
    if (!usb_func_config(&self->usb_func)) {
        error("config usb_func failed");
        return false;
    }

    // USBデバイスとして登録
    dev_name_service_add_dev(dev_name_service_get(), "eth10", self, false);

    // FIXME: ネットデバイスとして登録
    //netdev_add_dev(self);

    return true;
}

const char *cdcether_get_macaddr(cdcether_t *self)
{
    return self->macaddr;
}

boolean cdcether_send_frame(cdcether_t *self, const void *buffer, uint32_t len)
{
    // USB転送（バルク）
    return dw2_hc_xfer(usb_func_get_host(&self->usb_func), self->bulk_out, buffer, len) >= 0;
}

boolean cdcether_receive_frame(cdcether_t *self, void *buffer, uint32_t *resultlen)
{
    usb_req_t urb;
    usb_request(&urb, self->bulk_in, buffer, FRAME_BUFFER_SIZE, 0);
    urb.onnak = true;

    if (!dw2_hc_submit_block_request(usb_func_get_host(&self->usb_func), &urb, USB_TIMEOUT_NONE)) {
        debug("failed submit block request");
        return false;
    }

    uint32_t rlen = usb_req_get_resultlen(&urb);
    if (rlen == 0) {
        debug("resultlen is 0");
        return false;
    }
    *resultlen = rlen;
    return true;
}

boolean cdcether_init_macaddr(cdcether_t *self, uint8_t id)
{
    usb_str_t usb_str;
    usb_string(&usb_str, usb_func_get_dev(&self->usb_func));
    if (id == 0 || !usb_string_get_from_desc(&usb_str, id,
            usb_string_get_langid(&usb_str))) {
        debug("failed to get lang id");
        return false;
    }

    const char *addr = usb_string_get(&usb_str);

    uint8_t macaddr[MAC_ADDRESS_SIZE];
    for (unsigned i = 0; i < MAC_ADDRESS_SIZE; i++)
    {
        uint8_t byte = 0;
        for (unsigned j = 0; j < 2; j++)
        {
            char c = *addr++;
            if (c > '9') {
                c -= 'A'-'9'-1;
            }
            c -= '0';

            if (!('\0' <= c && c <= '\xF')) {
                return false;
            }

            byte <<= 4;
            byte |= (uint8_t) c;
        }

        macaddr[i] = byte;
    }

    memmove(self->macaddr, macaddr, MAC_ADDRESS_SIZE);

    return true;
}
