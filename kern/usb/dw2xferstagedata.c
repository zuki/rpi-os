//
// dwhcixferstagedata.c
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
#include "usb/dw2xferstagedata.h"
#include "usb/dw2fschedper.h"
#include "usb/dw2fschednper.h"
#include "usb/dw2fschednsplit.h"
#include "usb/dw2hc.h"
#include "types.h"
#include "console.h"
#include "linux/time.h"

#define MAX_BULK_TRIES      8

void dw2_xfer_stagedata(dw2_xfer_stagedata_t *self, unsigned channel, usb_req_t *urb, boolean in, boolean ststage, unsigned timeout)
{
    assert(self != 0);

    self->channel = channel;
    self->urb = urb;
    self->in = in;
    self->timeout = USB_TIMEOUT_NONE;
    self->ststage = ststage;
    self->split_comp = false;
    self->xfered = 0;
    self->state = 0;
    self->substate = 0;
    self->trstatus = 0;
    self->fsused = false;
    self->err_cnt = 0;
    self->start = 0;

    self->ep = usb_req_get_ep(urb);
    self->dev = usb_ep_get_device(self->ep);
    self->speed = usb_dev_get_speed(self->dev);
    self->xpsize = usb_ep_get_max_packet_size(self->ep);

    self->split = usb_dev_get_hubaddr(self->dev) != 0
               && self->speed != usb_speed_high;

    if (!ststage) {
        if (usb_ep_get_nextpid(self->ep, ststage) == usb_pid_setup) {
            self->buffp = usb_req_get_setup_data(urb);
            self->xfersize = sizeof(setup_data_t);
        } else {
            self->buffp = usb_req_get_buffer(urb);
            self->xfersize = usb_req_get_buflen(urb);
        }

        self->packets =(self->xfersize + self->xpsize - 1) / self->xpsize;

        if (self->split) {
            if (self->xfersize > self->xpsize) {
                self->bpt = self->xpsize;
            } else {
                self->bpt = self->xfersize;
            }

            self->ppt = 1;
        } else {
            self->bpt = self->xfersize;
            self->ppt = self->packets;
        }
    } else {
        self->buffp = &self->buffer;

        self->xfersize = 0;
        self->bpt = 0;
        self->packets = 1;
        self->ppt = 1;
    }

    assert(self->buffp != 0);
    trace("bp=0x%llx", self->buffp);
    if (((uintptr_t) self->buffp & 3) != 0) {
        warn("buffp not align: %p", self->buffp);
    }
    //assert(((uintptr_t) self->buffp & 3) == 0);

    if (self->split) {
        if (dw2_xfer_stagedata_is_periodic(self)) {
            dw2_fsched_per(&self->fsched.periodic);
        } else {
            dw2_fsched_nper(&self->fsched.nonperiodic);
        }
        self->fsused = true;
    } else {
        if (usb_dev_get_hubaddr(self->dev) == 0 && self->speed != usb_speed_high)
        {
            dw2_fsched_nsplit(&self->fsched.nosplit,
                            dw2_xfer_stagedata_is_periodic(self));
            self->fsused = true;
        }
    }

    if (timeout != USB_TIMEOUT_NONE) {
        assert(self->ep->type == ep_type_interrupt);

        self->timeout = timeout * HZ / 1000;
        self->start = jiffies;
    }
}

void _dw2_xfer_stagedata(dw2_xfer_stagedata_t *self)
{
    if (self->fsused) {
        self->fsched.base._fscheduler(&self->fsched.base);
    }

    self->buffp = 0;

    self->ep = 0;
    self->dev = 0;
    self->urb = 0;
}

void dw2_xfer_stagedata_trans_complete(dw2_xfer_stagedata_t *self, uint32_t status, uint32_t packetleft, uint32_t byteleft)
{
    self->trstatus = status;

    if (status & (DWHCI_HOST_CHAN_INT_ERROR_MASK
                | DWHCI_HOST_CHAN_INT_NAK
                | DWHCI_HOST_CHAN_INT_NYET)) {
        if (status & DWHCI_HOST_CHAN_INT_NAK && self->urb->onnak) {
            assert(self->in);   // IN転送であること
            self->packets = 0;  // 利用可能なデータなし、転送を完了
            return;
        }

        // xactエラーが生じたバルク転送は再試行する。それ以外はreturn
        if (!(status & DWHCI_HOST_CHAN_INT_XACT_ERROR)
           || self->ep->type != ep_type_bulk
           || ++self->err_cnt > MAX_BULK_TRIES) {
            return;
         }
    }

    uint32_t packetxfered = self->ppt - packetleft;
    uint32_t bytexfered   = self->bpt - byteleft;

    if (self->split && self->split_comp && bytexfered == 0 && self->bpt > 0) {
        bytexfered = self->xpsize * packetxfered;
    }

    self->xfered += bytexfered;
    self->buffp = (uint8_t *)self->buffp + bytexfered;

    if (!self->split || self->split_comp) {
        usb_ep_skip_pid(self->ep, packetxfered, self->ststage);
    }

    // これはないはずだが、何らかのデバイスで生じるようだ
    if (packetxfered > self->packets) {
        self->trstatus |= DWHCI_HOST_CHAN_INT_FRAME_OVERRUN;
        self->err_cnt = MAX_BULK_TRIES + 1;
        self->packets = 0;
        return;
    }

    self->packets -= packetxfered;

    if (!self->split) {
        self->ppt = self->packets;
    }

    // (xfersize > xfersize) の場合、これはfalseになる
    if (self->xfersize - self->xfered < self->bpt) {
        assert(self->xfered <= self->xfersize);
        self->bpt = self->xfersize - self->xfered;
    }
}

void dw2_xfer_stagedata_set_split_complete(dw2_xfer_stagedata_t *self, boolean comp)
{
    self->split_comp = comp;
}

void dw2_xfer_stagedata_set_state(dw2_xfer_stagedata_t *self, unsigned state)
{
    self->state = state;
}

unsigned dw2_xfer_stagedata_get_state(dw2_xfer_stagedata_t *self)
{
    return self->state;
}

void dw2_xfer_stagedata_set_sub_state(dw2_xfer_stagedata_t *self, unsigned substate)
{
    self->substate = substate;
}

unsigned dw2_xfer_stagedata_get_sub_state(dw2_xfer_stagedata_t *self)
{
    return self->substate;
}

/// @brief スプリットサイクルを開始する
/// @param self ステージデータ構造体へのポインタ
/// @return 開始したか
boolean dw2_xfer_stagedata_begin_split_cycle(dw2_xfer_stagedata_t *self)
{
    return true;
}

unsigned dw2_xfer_stagedata_get_channel_number(dw2_xfer_stagedata_t *self)
{
    return self->channel;
}

boolean dw2_xfer_stagedata_is_periodic(dw2_xfer_stagedata_t *self)
{
    ep_type_t type = usb_ep_get_type(self->ep);

    return type == ep_type_interrupt || type == ep_type_isochronous;
}

uint8_t dw2_xfer_stagedata_get_dev_addr(dw2_xfer_stagedata_t *self)
{
    return usb_dev_get_addr(self->dev);
}

uint8_t dw2_xfer_stagedata_get_ep_type(dw2_xfer_stagedata_t *self)
{
    unsigned type = 0;

    switch(usb_ep_get_type(self->ep))
    {
    case ep_type_control:
        type = DWHCI_HOST_CHAN_CHARACTER_EP_TYPE_CONTROL;
        break;

    case ep_type_bulk:
        type = DWHCI_HOST_CHAN_CHARACTER_EP_TYPE_BULK;
        break;

    case ep_type_interrupt:
        type = DWHCI_HOST_CHAN_CHARACTER_EP_TYPE_INTERRUPT;
        break;

    default:
        assert(0);
        break;
    }

    return type;
}

uint8_t dw2_xfer_stagedata_get_ep_number(dw2_xfer_stagedata_t *self)
{
    return usb_ep_get_number(self->ep);
}

uint32_t dw2_xfer_stagedata_get_max_packet_size(dw2_xfer_stagedata_t *self)
{
    return self->xpsize;
}

usb_speed_t dw2_xfer_stagedata_get_speed(dw2_xfer_stagedata_t *self)
{
    return self->speed;
}

uint8_t dw2_xfer_stagedata_get_pid(dw2_xfer_stagedata_t *self)
{
    uint8_t pid = 0;

    switch(usb_ep_get_nextpid(self->ep, self->ststage))
    {
    case usb_pid_setup:
        pid = DWHCI_HOST_CHAN_XFER_SIZ_PID_SETUP;
        break;

    case usb_pid_data0:
        pid = DWHCI_HOST_CHAN_XFER_SIZ_PID_DATA0;
        break;

    case usb_pid_data1:
        pid = DWHCI_HOST_CHAN_XFER_SIZ_PID_DATA1;
        break;

    default:
        assert(0);
        break;
    }

    return pid;
}

boolean dw2_xfer_stagedata_is_in(dw2_xfer_stagedata_t *self)
{
    return self->in;
}

boolean dw2_xfer_stagedata_is_ststage(dw2_xfer_stagedata_t *self)
{
    return self->ststage;
}

uint32_t dw2_xfer_stagedata_get_dmaaddr(dw2_xfer_stagedata_t *self)
{
    return (uint32_t)(uintptr_t)self->buffp;
}

uint32_t dw2_xfer_stagedata_get_bpt(dw2_xfer_stagedata_t *self)
{
    return self->bpt;
}

uint32_t dw2_xfer_stagedata_get_ppt(dw2_xfer_stagedata_t *self)
{
    return self->ppt;
}

boolean dw2_xfer_stagedata_is_split(dw2_xfer_stagedata_t *self)
{
    return self->split;
}

boolean dw2_xfer_stagedata_is_split_complete(dw2_xfer_stagedata_t *self)
{
    return self->split_comp;
}

uint8_t dw2_xfer_stagedata_get_hubaddr(dw2_xfer_stagedata_t *self)
{
    return usb_dev_get_hubaddr(self->dev);
}

uint8_t dw2_xfer_stagedata_get_hubport(dw2_xfer_stagedata_t *self)
{
     return usb_dev_get_hubport(self->dev);
}

uint8_t dw2_xfer_stagedata_get_split_pos(dw2_xfer_stagedata_t *self)
{
    // only important for isochronous transfers
    return DWHCI_HOST_CHAN_SPLIT_CTRL_ALL;
}

uint32_t dw2_xfer_stagedata_get_status_mask(dw2_xfer_stagedata_t *self)
{
    uint32_t mask =   DWHCI_HOST_CHAN_INT_XFER_COMPLETE
            | DWHCI_HOST_CHAN_INT_HALTED
            | DWHCI_HOST_CHAN_INT_ERROR_MASK;

    if (self->split || dw2_xfer_stagedata_is_periodic(self)) {
        mask |=   DWHCI_HOST_CHAN_INT_ACK
             | DWHCI_HOST_CHAN_INT_NAK
             | DWHCI_HOST_CHAN_INT_NYET;
    }

    return mask;
}

uint32_t dw2_xfer_stagedata_get_trstatus(dw2_xfer_stagedata_t *self)
{
    return self->trstatus;
}

boolean dw2_xfer_stagedata_is_stage_complete(dw2_xfer_stagedata_t *self)
{
    return self->packets == 0;
}

uint32_t dw2_xfer_stagedata_get_resultlen(dw2_xfer_stagedata_t *self)
{
    if (self->xfered > self->xfersize) {
        return self->xfersize;
    }

    return self->xfered;
}

usb_req_t *dw2_xfer_stagedata_get_urb(dw2_xfer_stagedata_t *self)
{
    return self->urb;
}

dw2_fsched_t *dw2_xfer_stagedata_get_fsched(dw2_xfer_stagedata_t *self)
{
    if (!self->fsused) {
        return 0;
    }

    return &self->fsched.base;
}

boolean dw2_xfer_stagedata_is_retry_ok(dw2_xfer_stagedata_t *self)
{
    return self->err_cnt <= MAX_BULK_TRIES;
}

usb_err_t dw2_xter_dtagedata_get_usb_err(dw2_xfer_stagedata_t *self)
{
    if (self->trstatus & DWHCI_HOST_CHAN_INT_STALL)
        return usb_err_stall;

    if (self->trstatus & DWHCI_HOST_CHAN_INT_XACT_ERROR)
        return usb_err_transaction;

    if (self->trstatus & DWHCI_HOST_CHAN_INT_BABBLE_ERROR)
        return usb_err_babble;

    if (self->trstatus & DWHCI_HOST_CHAN_INT_FRAME_OVERRUN)
        return usb_err_frame_overrun;

    if (self->trstatus & DWHCI_HOST_CHAN_INT_DATA_TOGGLE_ERROR)
        return usb_err_data_toggle;

    if (self->trstatus & DWHCI_HOST_CHAN_INT_AHB_ERROR)
        return usb_err_host_bus;

    return usb_err_unknown;
}

boolean dw2_xfer_stagedata_is_timeout(dw2_xfer_stagedata_t *self)
{
    if (self->timeout == USB_TIMEOUT_NONE)
        return false;

    return (jiffies - self->start) >= self->timeout ? true : false;
}
