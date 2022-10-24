//
// dwhciframeschednper.c
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
#include "usb/dw2fschednper.h"
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
    fsched_state_unknown
} fsched_state_t;

void dw2_fsched_nper(dw2_fsched_nper_t *self)
{
    dw2_fsched_t *base =(dw2_fsched_t *)self;

    base->_fscheduler = _dw2_fsched_nper;
    base->start_split = dw2_fsched_nper_start_split;
    base->complete_split = dw2_fsched_nper_complete_split;
    base->transaction_complete = dw2_fsched_nper_transaction_complete;
#ifndef USE_USB_SOF_INTR
    base->wait_for_frame = dw2_fsched_nper_wait_for_frame;
#else
    base->get_frame_number = dw2_fsched_nperget_frame_number;
    base->periodic_delay = w2_fsched_nper_periodic_delay;
#endif
    base->is_odd_frame = dw2_fsched_nper_is_odd_frame;

    self->state = fsched_state_unknown;
#ifdef USE_USB_SOF_INTR
    self->offset = 0;
#endif
}

void _dw2_fsched_nper(dw2_fsched_t *base)
{
    dw2_fsched_nper_t *self = (dw2_fsched_nper_t *) base;
    self->state = fsched_state_unknown;
}

void dw2_fsched_nper_start_split(dw2_fsched_t *base)
{
    dw2_fsched_nper_t *self = (dw2_fsched_nper_t *) base;

#ifdef USE_USB_SOF_INTR
    if (self->state != fsched_state_comp_split_failed)
        self->offset = 1;
#endif

    self->state = fsched_state_start_split;
}

boolean dw2_fsched_nper_complete_split(dw2_fsched_t *base)
{
    dw2_fsched_nper_t *self =(dw2_fsched_nper_t *) base;
    boolean result = false;

#ifdef USE_USB_SOF_INTR
    self->offset = 2;
#endif

    switch(self->state) {
    case fsched_state_start_split_comp:
        self->state = fsched_state_comp_split;
        self->tries = 3;
        result = true;
        break;

    case fsched_state_comp_split:
    case fsched_state_comp_retry:
#ifndef USE_USB_SOF_INTR
        delayus(5 * FRAME);
#else
        self->offset = 1;
#endif
        result = true;
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

void dw2_fsched_nper_transaction_complete(dw2_fsched_t *base, uint32_t status)
{
    dw2_fsched_nper_t *self =(dw2_fsched_nper_t *) base;

    switch(self->state)
    {
    case fsched_state_start_split:
        assert(status & DWHCI_HOST_CHAN_INT_ACK);
        self->state = fsched_state_start_split_comp;
        break;

    case fsched_state_comp_split:
    case fsched_state_comp_retry:
        if(status & DWHCI_HOST_CHAN_INT_XFER_COMPLETE) {
            self->state = fsched_state_comp_split_comp;
        } else if(status &(DWHCI_HOST_CHAN_INT_NYET | DWHCI_HOST_CHAN_INT_ACK)) {
            if(self->tries-- == 0) {
#ifdef USE_USB_SOF_INTR
                self->offset = 1;
#endif
                self->state = fsched_state_comp_split_failed;
            } else {
                self->state = fsched_state_comp_retry;
            }
        } else if(status & DWHCI_HOST_CHAN_INT_NAK) {
#ifndef USE_USB_SOF_INTR
            delayus(5 * FRAME);
#else
            self->offset = 5;
#endif
            self->state = fsched_state_comp_split_failed;

        } else {
            error("Invalid status 0x%x", status);
            assert(0);
        }
        break;

    default:
        assert(0);
        break;
    }
}

#ifndef USE_USB_SOF_INTR

void dw2_fsched_nper_wait_for_frame(dw2_fsched_t *base)
{
    // nop
}

#else

uint16_t dw2_fsched_nper_get_frame_number(dw2_fsched_t *base)
{
    dw2_fsched_nper_t *self =(dw2_fsched_nper_t *) base;

    uint32_t framnum = get32(DWHCI_HOST_FRM_NUM);
    uint16_t fnum = DWHCI_HOST_FRM_NUM_NUMBER(framnum);

    assert (self->offset < 8);
	return (fnum + self->ofsser) & DWHCI_MAX_FRAME_NUMBER;
}

void w2_fsched_nper_periodic_delay(dw2_fsched_t *base, uint16_t offset)
{
	assert (0);
}

#endif

boolean dw2_fsched_nper_is_odd_frame(dw2_fsched_t *base)
{
    return false;
}
