//
// usbstring.h
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
#ifndef INC_USB_STRING_H
#define INC_USB_STRING_H

#include "usb/usb.h"
#include "types.h"
#include "string.h"

struct usb_dev;

typedef struct usb_string {
    struct usb_dev  *dev;
    str_desc_t      *str_desc;
    char            *str;
} usb_str_t;

void usb_string(usb_str_t *self, struct usb_dev *dev);
void usb_string_copy(usb_str_t *self, usb_str_t *parent);
void _usb_string(usb_str_t *self);

boolean usb_string_get_from_desc(usb_str_t *self, uint8_t id, uint16_t langid);
const char *usb_string_get(usb_str_t *self);
uint16_t usb_string_get_langid(usb_str_t *self);

#endif
