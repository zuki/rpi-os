//
// usb_req_.c
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
#include "usb/request.h"
#include "types.h"
#include "console.h"

void usb_request(usb_req_t *self, usb_ep_t *ep, void *buffer, uint32_t buflen, setup_data_t *setup_data)
{
    self->ep            = ep;
    self->setup_data    = setup_data;
    self->buffer        = buffer;
    self->buflen        = buflen;
    self->status        = 0;
    self->resultlen     = 0;
    self->cb            = 0;
    self->param         = 0;
    self->ctx           = 0;
    self->onnak         = false;
    self->error         = usb_err_unknown;
}

void _usb_request(usb_req_t *self)
{
    self->ep            = 0;
    self->setup_data    = 0;
    self->buffer        = 0;
    self->cb            = 0;
}

usb_ep_t *usb_req_get_ep(usb_req_t *self)
{
    return self->ep;
}

void usb_req_set_status(usb_req_t *self, int status)
{
    self->status = status;
}

void usb_req_set_resultlen(usb_req_t *self, uint32_t resultlen)
{
    self->resultlen = resultlen;
}

int usb_req_get_status(usb_req_t *self)
{
    return self->status;
}

uint32_t usb_req_get_resultlen(usb_req_t *self)
{
    return self->resultlen;
}

setup_data_t *usb_req_get_setup_data(usb_req_t *self)
{
    return self->setup_data;
}

void *usb_req_get_buffer(usb_req_t *self)
{
    return self->buffer;
}

uint32_t usb_req_get_buflen(usb_req_t *self)
{
    return self->buflen;
}

void usb_req_set_comp_cb(usb_req_t *self, usb_comp_cb *cb, void *param, void *ctx)
{
    self->cb    = cb;
    self->param = param;
    self->ctx   = ctx;
}

void usb_req_call_comp_cb(usb_req_t *self)
{
    (*self->cb)(self, self->param, self->ctx);
}

void usb_req_set_comp_on_nak(usb_req_t *self)
{
    self->onnak = true;
}

boolean usb_req_is_comp_on_nak(usb_req_t *self)
{
    return self->onnak;
}
