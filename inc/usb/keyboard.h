//
// usbkeyboard.h
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
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef INC_USB_KEYBOARD_H
#define INC_USB_KEYBOARD_H

#include "usb/function.h"
#include "usb/endpoint.h"
#include "usb/request.h"
#include "usb/keymap.h"
#include "types.h"

#define BOOT_REPORT_SIZE    8

typedef void key_pressed_hdl(const char *str);
typedef void sel_console_hdl(unsigned console);
typedef void shutdown_hdl(void);

// The raw handler is called when the keyboard sends a status report (on status change and/or continously).
typedef void key_status_hdl (
    unsigned char modifier,             // see usbhid.h
    const unsigned char  rawkeys[6]);   // key code or 0 in each byte

typedef struct usb_keyborad {
    usb_func_t          func;
    usb_ep_t           *ep;
    key_pressed_hdl    *pressed_handler;
    sel_console_hdl    *sel_handler;
    shutdown_hdl       *shutdown_handler;
    key_status_hdl     *status_handler;
    usb_req_t           urb;
    uint8_t            *buffer;
    uint8_t             pcode;
    unsigned            timer;
    keymap_t            keymap;
    uint8_t             ledstatus;
} usb_kbd_t;

void usb_keyboard(usb_kbd_t *self, usb_func_t *func);
void _usb_keyboard(usb_kbd_t *self);

boolean kbd_config(usb_func_t *func);

// cooked mode
void kbd_register_key_pressed_hdl(usb_kbd_t *self, key_pressed_hdl *handler);
void kbd_register_sel_console_hdl(usb_kbd_t *self, sel_console_hdl *handler);
void kbd_register_shutdown_hdl(usb_kbd_t *self, shutdown_hdl *handler);

void kbd_update_led(usb_kbd_t *self);

// raw mode (if this handler is registered the others are ignored)
void kbd_register_key_status_hdl(usb_kbd_t *self, key_status_hdl *handler);

void kbd_set_led(usb_kbd_t *self, uint8_t mask);

#endif
