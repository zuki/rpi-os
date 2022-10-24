//
// dwhciframeschednsplit.h
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
#ifndef INC_USB_DW2FSCHEDNSPLIT_H
#define INC_USB_DW2FSCHEDNSPLIT_H

#include "usb/dw2fscheduler.h"
#include "types.h"

typedef struct dw2_framescheuler_non_split {
    dw2_fsched_t    scheduler;
    boolean         periodic;
    unsigned        next;
} dw2_fsched_nsplit_t;

void dw2_fsched_nsplit(dw2_fsched_nsplit_t *self, boolean periodic);
void _dw2_fsched_nsplit(dw2_fsched_t *base);

void dw2_fsched_nsplit_start_split(dw2_fsched_t *base);
boolean dw2_fsched_nsplit_complete_split(dw2_fsched_t *base);
void dw2_fsched_nsplit_transaction_complete(dw2_fsched_t *base, uint32_t status);

#ifndef USE_USB_SOF_INTR
void dw2_fsched_nsplit_wait_for_frame(dw2_fsched_t *base);
#else
uint16_t dw2_fsched_nsplit_get_frame_number(dw2_fsched_t *base);
void w2_fsched_nsplit_periodic_delay(dw2_fsched_t *base, uint16_t offset);
#endif

boolean dw2_fsched_nsplit_is_odd_frame(dw2_fsched_t *base);

#endif
