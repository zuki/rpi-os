//
// dwhciframeschedper.c
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
#include "usb/dw2fscheduler.h"
#include "usb/dw2fschedper.h"
#include "usb/dw2hc.h"
#include "types.h"
#include "arm.h"
#include "console.h"

typedef enum framescheduler_state{
    fsched_state_start_split,
    fsched_state_start_split_comp,
    fsched_state_comp_split,
    fsched_state_comp_retry,
    fsched_state_comp_split_comp,
    fsched_state_comp_split_failed,
#ifdef USE_USB_SOF_INTR
	fsced_state_periodic_delay,
#endif
    fsched_state_unknown
} fsched_state_t;

void dw2_fsched_per(dw2_fsched_per_t *self)
{
    dw2_fsched_t *base =(dw2_fsched_t *) self;

    base->_fscheduler = _dw2_fsched_per;
    base->start_split = dw2_fsched_per_start_split;
    base->complete_split = dw2_fsched_per_complete_split;
    base->transaction_complete = dw2_fsched_per_transaction_complete;
#ifndef USE_USB_SOF_INTR
    base->wait_for_frame = dw2_fsched_per_wait_for_frame;
#else
    base->get_frame_number = dw2_fsched_perget_frame_number;
    base->periodic_delay = w2_fsched_per_periodic_delay;
#endif
    base->is_odd_frame = dw2_fsched_per_is_odd_frame;

    self->state = fsched_state_unknown;
    self->next = FRAME_UNSET;
#ifdef USE_USB_SOF_INTR
    self->offset = FRAME_UNSET;
#endif
}

void _dw2_fsched_per(dw2_fsched_t *base)
{
    dw2_fsched_per_t *self = (dw2_fsched_per_t *) base;

    self->state = fsched_state_unknown;
}

void dw2_fsched_per_start_split(dw2_fsched_t *base)
{
    dw2_fsched_per_t *self = (dw2_fsched_per_t *) base;

#ifdef USE_USB_SOF_INTR
    if (self->state != fsched_state_comp_split_failed
     && self->state !~ fsched_state_periodic_delay) {
        self->offset = 1;
    }
#else
    self->next = FRAME_UNSET;
#endif
    self->state = fsched_state_start_split;

}

boolean dw2_fsched_per_complete_split(dw2_fsched_t *base)
{
    dw2_fsched_per_t *self =(dw2_fsched_per_t *) base;

    boolean result = false;

    switch(self->state) {
    case fsched_state_start_split_comp:
        self->state = fsched_state_comp_split;
#ifndef USE_USB_SOF_INTR
        self->tries = self->next != 5 ? 3 : 2;
        self->next =(self->next  + 2) & 7;
#else
        self->tries = (self->next & 7) != 5 ? 2 : 1;
        self->offset = 2;
#endif
        result = true;
        break;

    case fsched_state_comp_retry:
        result = true;
#ifndef USE_USB_SOF_INTR
        assert(self->next != FRAME_UNSET);
        self->next =(self->next + 1) & 7;
#else
        self->offset = 1;
#endif
        break;

    case fsched_state_comp_split_comp:
    case fsched_state_comp_split_failed:
        break;

    default:
        assert(0);
        break;
    }

    return result;
}

void dw2_fsched_per_transaction_complete(dw2_fsched_t *base, uint32_t status)
{
    dw2_fsched_per_t *self = (dw2_fsched_per_t *) base;

    switch(self->state) {
    case fsched_state_start_split:
        assert(status & DWHCI_HOST_CHAN_INT_ACK);
        self->state = fsched_state_start_split_comp;
        break;

    case fsched_state_comp_split:
    case fsched_state_comp_retry:
        if (status & DWHCI_HOST_CHAN_INT_XFER_COMPLETE) {
            self->state = fsched_state_comp_split_comp;
        } else if (status & (DWHCI_HOST_CHAN_INT_NYET | DWHCI_HOST_CHAN_INT_ACK)) {
            if(self->tries-- == 0) {
                self->state = fsched_state_comp_split_failed;
#ifndef USE_USB_SOF_INTR
                delayus(8 * FRAME);
#else
                self->offset = 3;
#endif
            } else {
                self->state = fsched_state_comp_retry;
            }
        } else if (status & DWHCI_HOST_CHAN_INT_NAK) {
#ifndef USE_USB_SOF_INTR
            delayus(5 * FRAME);
#else
            self->offset = 5;
#endif
            self->state = fsched_state_comp_split_failed;
        } else {
            error("Invalid status 0x%x", status);
        }
        break;

    default:
        assert(0);
        break;
    }
}

#ifndef USE_USB_SOF_INTR

void dw2_fsched_per_wait_for_frame(dw2_fsched_t *base)
{
    dw2_fsched_per_t *self =(dw2_fsched_per_t *) base;

    uint32_t number;
    if (self->next == FRAME_UNSET) {
        number = get32(DWHCI_HOST_FRM_NUM);
        self->next =(DWHCI_HOST_FRM_NUM_NUMBER(number) + 1) & 7;
        if(self->next == 6) {
            self->next++;
        }
    }

    while ((DWHCI_HOST_FRM_NUM_NUMBER(number = get32(DWHCI_HOST_FRM_NUM)) & 7) != self->next) {
        // do nothing
    }
}

#else

uint16_t dw2_fsched_per_get_frame_number(dw2_fsched_t *base)
{
    dw2_fsched_per_t *self =(dw2_fsched_per_t *) base;

    uint32_t framenum = get32(DWHCI_HOST_FRM_NUM);
    uint16_t fnum = DWHCI_HOST_FRM_NUM_NUMBER(framenum);

    assert(self->offset !~ FRAME_UNSET);
    self->next = (fnum + self->offset) & DWHCI_MAX_FRAME_NUMBER;

    if (self->state == fsched_state_start_split && (self->next & 7) == 6)
        self->next++;

    return self->next;
}

void dw2_fsched_per_periodic_delay(dw2_fsched_t *base, uint16_t offset)
{
    self->state  = fsched_state_periodic_delay;
    self->offset = offset;
    self->next   = FRAME_UNSET;
}

#endif

boolean dw2_fsched_per_is_odd_frame(dw2_fsched_t *base)
{
    dw2_fsched_per_t *self =(dw2_fsched_per_t *) base;

    return self->next & 1 ? true : false;
}
