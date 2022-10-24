//
// dwhcirootport.cpp
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
// along with this program. pThis If not, see <http://www.gnu.org/licenses/>.
//
#include "usb/dw2rootport.h"
#include "usb/dw2hcd.h"
#include "usb/device.h"
#include "types.h"
#include "kmalloc.h"
#include "console.h"


void dw2_rport(dw2_rport_t *self, dw2_hc_t *host)
{
    self->host  = host;
    self->dev   = 0;
}

void _dw2_rport(dw2_rport_t *self)
{
    if (self->dev != 0) {
        _usb_device(self->dev);
        kmfree(self->dev);
        self->dev = 0;
    }
    self->host = 0;
}

boolean dw2_rport_init(dw2_rport_t *self)
{
    usb_speed_t speed = dw2_hc_get_port_speed(self->host);
    if (speed == usb_speed_unknown) {
        error("cannot detect port speed");
        return false;
    }

    // first create default device
    self->dev = (usb_dev_t *)kmalloc(sizeof(usb_dev_t));
    usb_device(self->dev, self->host, speed, false, 0, 1);

    if (!usb_dev_init(self->dev)) {
        _usb_device(self->dev);
        kmfree(self->dev);
        self->dev = 0;
        return false;
    }

    if (!usb_dev_config(self->dev)) {
        error("cannot configure device");
        _usb_device(self->dev);
        kmfree(self->dev);
        self->dev = 0;
        return false;
    }

    debug("Device configured");

    // check for over-current
    if (dw2_hc_overcurrent_detected(self->host)) {
        error("Over-current condition");
        dw2_hc_disable_rport(self->host, true);
        _usb_device(self->dev);
        kmfree(self->dev);
        self->dev = 0;
        return false;
    }

    return true;
}

boolean dw2_rport_rescan_dev(dw2_rport_t *self)
{
    if (self->dev == 0) {
        warn("Previous attempt to initialize device failed");
        return false;
    }

    return usb_dev_rescan_dev(self->dev);
}

boolean dw2_rport_remove_dev(dw2_rport_t *self)
{
    dw2_hc_disable_rport(self->host, false);
    kmfree(self->dev);
    self->dev = 0;
    return true;
}

void dw2_rport_handle_port_status_change(dw2_rport_t *self)
{
    if (dw2_hc_device_connected(self->host)) {
        if (self->dev == 0)
            dw2_hc_rescan_dev(self->host);
    } else {
        if (self->dev != 0)
            dw2_hc_disable_rport(self->host, true);
    }
}

void dw2_rport_port_status_changed(dw2_rport_t *self)
{
/*
    if (dw2_hc_is_pap(self->host))
    {
        pst_ev_t *event = (pst_ev_t*)kmalloc(sizeof(pst_ev_t));
        assert (event != 0);
        event->fromrp = true;
        event->rport  = self;
        list_init(&event->list);

        acquire(&self->host->hublock);
        list_push_back(self->host->hublist, &event->list);
        release(&self->host->hublock);
    }
*/
}
