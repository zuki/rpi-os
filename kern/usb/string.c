//
// usb_string.c
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
#include "usb/string.h"
#include "usb/usb.h"
#include "usb/device.h"
#include "usb/dw2hcd.h"
#include "types.h"
#include "console.h"
#include "kmalloc.h"
#include "string.h"

#define USBSTR_MIN_LENGTH       4
#define USBSTR_DEFAULT_LANGID   0x409

void usb_string(usb_str_t *self, usb_dev_t *dev)
{
    self->dev = dev;
    self->str_desc = 0;
    self->str = (char *)kmalloc(256);
}

void usb_string_copy(usb_str_t *self, usb_str_t *parent)
{
    self->dev = parent->dev;
    self->str = 0;
    if (parent->str_desc != 0) {
        self->str_desc = (str_desc_t *)kmalloc(parent->str_desc->len);
        memmove(self->str_desc, parent->str_desc, parent->str_desc->len);
    }
    self->str = (char *)kmalloc(strlen(parent->str) + 1);
    safestrcpy(self->str, parent->str, strlen(parent->str));
}

void _usb_string(usb_str_t *self)
{
    kmfree(self->str);
    self->str = 0;

    if (self->str_desc != 0) {
        kmfree(self->str_desc);
        self->str_desc = 0;
    }

    self->dev = 0;
}

boolean usb_string_get_from_desc(usb_str_t *self, uint8_t id, uint16_t langid)
{
    if (self->str_desc != 0) {
        kmfree (self->str_desc);
    }

    self->str_desc = (str_desc_t *)kmalloc(USBSTR_MIN_LENGTH);
    if (dw2_hc_control_message(self->dev->host, self->dev->ep0,
            REQUEST_IN, GET_DESCRIPTOR,
            (DESCRIPTOR_STRING << 8) | id, langid,
            self->str_desc, USBSTR_MIN_LENGTH) < 0) {
        debug("failed to get DESCRIPTOR_STRING");
        return false;
    }

    uint8_t len = self->str_desc->len;
    if (len < 2 || (len & 1) != 0 || self->str_desc->type != DESCRIPTOR_STRING) {
        debug("bad DESCRIPTOR_STRING");
        return false;
    }

    if (len > USBSTR_MIN_LENGTH) {
        kmfree(self->str_desc);
        self->str_desc = (str_desc_t *)kmalloc(len);
        if (dw2_hc_control_message(self->dev->host, self->dev->ep0,
            REQUEST_IN, GET_DESCRIPTOR,
            (DESCRIPTOR_STRING << 8) | id, langid,
            self->str_desc, len) != (int)len) {
            debug("failed to get detailed DESCRIPTOR_STRING");
            return false;
        }

        if (self->str_desc->len != len
        || (self->str_desc->len & 1) != 0
        || self->str_desc->type != DESCRIPTOR_STRING) {
            debug("bad detailed DESCRIPTOR_STRING");
            return false;
        }
    }

    // convert to ASCII string
    size_t length = (self->str_desc->len - 2) / 2;
    char buffer[length+1];

    for (unsigned i = 0; i < length; i++) {
        uint16_t nc = self->str_desc->string[i];
        if (nc < ' ' || nc > '~') {
            nc = '_';
        }
        buffer[i] = (char) nc;
    }
    buffer[length] = '\0';

    kmfree(self->str);
    self->str = (char *)kmalloc(length+1);
    safestrcpy(self->str, buffer, length);

    return true;
}

const char *usb_string_get(usb_str_t *self)
{
    return self->str;
}

uint16_t usb_string_get_langid(usb_str_t *self)
{
    str_desc_t *langids = (str_desc_t *)kmalloc(USBSTR_MIN_LENGTH);
    if (dw2_hc_get_desc(self->dev->host, self->dev->ep0,
            DESCRIPTOR_STRING, 0, langids,
            USBSTR_MIN_LENGTH, REQUEST_IN, 0) < 0) {
        kmfree(langids);
        return USBSTR_DEFAULT_LANGID;
    }

    uint8_t len = langids->len;
    if (len < 4 || (len & 1) != 0 || langids->type != DESCRIPTOR_STRING) {
        kmfree(langids);
        return USBSTR_DEFAULT_LANGID;
    }

    if (len > USBSTR_MIN_LENGTH) {
        kmfree(langids);
        langids = (str_desc_t *)kmalloc(len);
        if (dw2_hc_get_desc(self->dev->host, self->dev->ep0,
                DESCRIPTOR_STRING, 0, langids,
                len, REQUEST_IN, 0) != (int)len) {
            kmfree(langids);
            return USBSTR_DEFAULT_LANGID;
        }

        if (langids->len != len || (langids->len & 1) != 0
         || langids->type != DESCRIPTOR_STRING) {
            kmfree(langids);
            return USBSTR_DEFAULT_LANGID;
        }
    }

    size_t length = (langids->len - 2) / 2;
    // search for default language ID
    for (unsigned i = 0; i < length; i++) {
        if (langids->string[i] == USBSTR_DEFAULT_LANGID) {
            kmfree(langids);
            return USBSTR_DEFAULT_LANGID;
        }
    }

    // default language ID not found, use first ID
    uint16_t result = langids->string[0];
    kmfree(langids);
    return result;
}
