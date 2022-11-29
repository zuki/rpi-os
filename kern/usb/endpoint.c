//
// usbendpoint.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright(C) 2014  R. Stange <rsta2@o2online.de>
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
#include "usb/endpoint.h"
#include "types.h"
#include "console.h"

void usb_endpoint(usb_ep_t *self, usb_dev_t *dev)
{
    assert(self != 0);
    self->dev = dev;
    self->num = 0;
    self->type = ep_type_control;
    self->in = false;
    self->xsize = USB_DEFAULT_MAX_PACKET_SIZE;
    self->interval = 1;
    self->nextpid = usb_pid_setup;
}

void usb_endpoint2(usb_ep_t *self, usb_dev_t *dev, const ep_desc_t *desc)
{
    self->dev = dev;
    self->interval = 1;
    self->nextpid = usb_pid_data0;
    assert(desc->length >= sizeof *desc); // クラス固有トレイラがある場合があるので>=
    assert(desc->type == DESCRIPTOR_ENDPOINT);

    switch(desc->attr & 0x03) {
    case 2:
        self->type = ep_type_bulk;
        break;

    case 3:
        self->type = ep_type_interrupt;
        break;

    default:
        assert(0);  // エンドポイントコンフィグレーションは
        return;     // 属性クラスのドライバでチェックする
    }

    self->num       = desc->addr & 0x0F;
    self->in        = desc->addr & 0x80 ? true : false;
    self->xsize     = desc->xsize & 0x7FF;

    // インタラプト転送
    if (self->type == ep_type_interrupt) {
        uint8_t interval = desc->interval;
        if (interval < 1) {
            interval = 1;
        }

        // see USB 2.0 spec chapter 9.6.6
        if(self->dev->speed < usb_speed_high) {    // LS/FS
            self->interval = interval;
        } else {                                    // HS
            if (interval > 16) {
                interval = 16;
            }
            unsigned value = 1 << (interval - 1);
            self->interval = value / 8;
            if(self->interval < 1) {
                self->interval = 1;
            }
        }
#ifndef USE_USB_SOF_INTR
        // interval 20ms is minimum to reduce interrupt rate
        if (self->interval < 20)
        {
            self->interval = 20;
        }
#endif
    }

    // バルクエンドポイントでLPの場合の回避策、通常、仕様では禁止されている
    if (self->dev->speed == usb_speed_low && self->type == ep_type_bulk) {
        warn("Device is not fully USB compliant");
        // 割り込みEPにする
        self->type = ep_type_interrupt;
        // 最大パケットサイズは8以下
        if (self->xsize > 8)
            self->xsize = 8;

#ifdef USE_USB_SOF_INTR
        self->interval = 1;
#else
        self->interval = 20;
#endif
    }

}

void usb_endpoint_copy(usb_ep_t *self, usb_ep_t *ep, usb_dev_t *dev)
{
    self->dev       = dev;
    self->num       = ep->num;
    self->type      = ep->type;
    self->in        = ep->in;
    self->xsize     = ep->xsize;
    self->interval  = ep->interval;
    self->nextpid   = ep->nextpid;
}

void _usb_endpoint(usb_ep_t *self)
{
    self->dev = 0;
}

usb_dev_t *usb_ep_get_device(usb_ep_t *self)
{
    return self->dev;
}

uint8_t usb_ep_get_number(usb_ep_t *self)
{
    return self->num;
}

ep_type_t usb_ep_get_type(usb_ep_t *self)
{
    return self->type;
}

boolean usb_ep_is_direction_in(usb_ep_t *self)
{
    return self->in;
}

void usb_ep_set_max_packet_size(usb_ep_t *self, uint32_t xsize)
{
    self->xsize = xsize;
}

uint32_t usb_ep_get_max_packet_size(usb_ep_t *self)
{
    return self->xsize;
}

unsigned usb_ep_get_interval(usb_ep_t *self)
{
    assert(self->type == ep_type_interrupt);
    return self->interval;
}

usb_pid_t usb_ep_get_nextpid(usb_ep_t *self, boolean ststatus)
{
    if (ststatus) {
        assert(self->type == ep_type_control);
        return usb_pid_data1;
    }

    return self->nextpid;
}

void usb_ep_skip_pid(usb_ep_t *self, unsigned packets, boolean ststatus)
{
    if (!ststatus) {
        switch(self->nextpid) {
        case usb_pid_setup:
            self->nextpid = usb_pid_data1;
            break;

        case usb_pid_data0:
            if (packets & 1)
                self->nextpid = usb_pid_data1;
            break;

        case usb_pid_data1:
            if (packets & 1)
                self->nextpid = usb_pid_data0;
            break;

        default:
            info("bad next pid: %d", self->nextpid);
            assert(0);
            break;
        }
    } else {
        if (self->type != ep_type_control) warn("type = %d", self->type);
        //assert(self->type == ep_type_control);
        self->nextpid = usb_pid_setup;
    }
}

void usb_ep_reset_pid(usb_ep_t *self)
{
    assert(self->type == ep_type_bulk);
    self->nextpid = usb_pid_data0;
}
