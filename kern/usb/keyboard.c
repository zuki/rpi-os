//
// usbkeyboard.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
//
// This program is kmfree software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the kmfree Software Foundation, either version 3 of the License, or
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
#include "usb/keyboard.h"
#include "usb/hid.h"
#include "usb/dw2hcd.h"
#include "usb/devicenameservice.h"
#include "types.h"
#include "console.h"
#include "kmalloc.h"
#include "string.h"

//#define REPEAT_ENABL      // does not work well with any Keyboard

#define MSEC2HZ(msec)       ((msec) * HZ / 1000)

#define REPEAT_DELAY        MSEC2HZ (400)
#define REPEAT_RATE         MSEC2HZ (80)

static unsigned devno = 1;

static void kbd_generate_key_event(usb_kbd_t *self, uint8_t pcode);
static boolean kbd_start_request(usb_kbd_t *self);
static void kbd_comp_cb(usb_req_t *urb, void *param, void *ctx);
static uint8_t kbd_get_modifiers(usb_kbd_t *self);
static uint8_t kbd_get_kcode(usb_kbd_t *self);
#ifdef REPEAT_ENABLE
static void kbd_timer_hdl(unsigned hTimer, void *param, void *ctx);
#endif

void usb_keyboard(usb_kbd_t *self, usb_func_t *func)
{
    assert (self != 0);

    usb_func_copy(&self->func, func);
    self->func.configure = kbd_config;

    self->ep = 0;
    self->pressed_handler = 0;
    self->sel_handler = 0;
    self->shutdown_handler = 0;
    self->status_handler = 0;
    self->buffer = 0;
    self->pcode = 0;
    self->timer = 0;
    self->ledstatus = 0;

    keymap(&self->keymap);

    self->buffer = kmalloc(BOOT_REPORT_SIZE);
    assert(self->buffer != 0);
}

void _usb_keyboard(usb_kbd_t *self)
{
    assert(self != 0);

    if (self->buffer != 0) {
        kmfree(self->buffer);
        self->buffer = 0;
    }

    if (self->ep != 0) {
        _usb_endpoint(self->ep);
        kmfree(self->ep);
        self->ep = 0;
    }

    _keymap(&self->keymap);
    _usb_function(&self->func);
}

boolean kbd_config(usb_func_t *func)
{
    // 1. usb_standardhubオブジェクトを取り出し
    usb_kbd_t *self = (usb_kbd_t *)func;
    assert (self != 0);
    // 2. EPの数は1以上であること
    if (usb_func_get_num_eps(&self->func) < 1) {
        warn("failed getting nums of endpoints");
        return false;
    }
    // 3. エンドポイントディスクリプタを取得とエンドポイントの作成
    ep_desc_t *ep_desc;
    while ((ep_desc = (ep_desc_t *)
            usb_func_get_desc(&self->func, DESCRIPTOR_ENDPOINT)) != 0) {
        if ((ep_desc->addr & 0x80) != 0x80       // Input EP
         || (ep_desc->attr & 0x3F) != 0x03) {    // Interrupt EP
            continue;
        }

        assert (self->ep == 0);
        self->ep = kmalloc(sizeof(usb_ep_t));
        assert (self->ep != 0);
        usb_endpoint2(self->ep, usb_func_get_dev(&self->func), ep_desc);
        break;
    }
    if (self->ep == 0) {
        warn("cannot get ep");
        return false;
    }
    // 4. ファンクションクラスとしてConfigure
    if (!usb_func_config(&self->func)) {
        warn("cannot set interface");
        return false;
    }
    // 5.
    dw2_hc_t *host = usb_func_get_host(&self->func);
    if (dw2_hc_control_message(host, usb_func_get_ep0(&self->func),
            REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
            SET_PROTOCOL, BOOT_PROTOCOL,
            usb_func_get_if_num(&self->func), 0, 0) < 0) {
        warn("Cannot set boot protocol");
        return false;
    }

    // 6. setting the LED status forces some keyboard adapters to work
    kbd_set_led(self, self->ledstatus);
    // 7. デバイス番号を取得してデバイスを登録
    char *name = (char *)kmalloc(16);
    sprintf(name, "ukbd%x", devno++);
    dev_name_service_add_dev(dev_name_service_get(), name, self, false);
    kmfree(name);

    return kbd_start_request(self);
}

void kbd_register_key_pressed_hdl(usb_kbd_t *self, key_pressed_hdl *handler)
{
    assert (self != 0);
    assert (handler != 0);
    self->pressed_handler = handler;
}

void kbd_register_sel_console_hdl(usb_kbd_t *self, sel_console_hdl *handler)
{
    assert (self != 0);
    assert (handler != 0);
    self->sel_handler = handler;
}

void kbd_register_shutdown_hdl(usb_kbd_t *self, shutdown_hdl *handler)
{
    assert (self != 0);
    assert (handler != 0);
    self->shutdown_handler = handler;
}

void kbd_update_led(usb_kbd_t *self)
{
    assert (self != 0);
    uint8_t lstatus = km_get_led_status(&self->keymap);
    if (lstatus != self->ledstatus) {
        kbd_set_led(self, lstatus);
        self->ledstatus = lstatus;
    }
}

void kbd_register_key_status_hdl(usb_kbd_t *self, key_status_hdl *handler)
{
    assert (self != 0);
    assert (handler != 0);
    self->status_handler = handler;
}

void kbd_set_led(usb_kbd_t *self, uint8_t mask)
{
    assert (self != 0);
    uint8_t leds[1] GALIGN(4) = {mask};    // DMA buffer

    dw2_hc_t *host = usb_func_get_host(&self->func);
    if (dw2_hc_control_message(host, usb_func_get_ep0(&self->func),
            REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
            SET_REPORT, (REPORT_TYPE_OUTPUT << 8) | 0,
            usb_func_get_if_num(&self->func), leds, sizeof leds) < 0) {
        warn("Cannot set led");
    }
}

void kbd_generate_key_event(usb_kbd_t *self, uint8_t pcode)
{
    assert (self != 0);
    const char *kstr;
    char buffer[2];

    uint8_t modifiers = kbd_get_modifiers(self);
    uint16_t lcode = km_translate(&self->keymap, pcode, modifiers);

    switch (lcode)
    {
    case ActionSwitchCapsLock:
    case ActionSwitchNumLock:
    case ActionSwitchScrollLock:
        break;

    case ActionSelectConsole1:
    case ActionSelectConsole2:
    case ActionSelectConsole3:
    case ActionSelectConsole4:
    case ActionSelectConsole5:
    case ActionSelectConsole6:
    case ActionSelectConsole7:
    case ActionSelectConsole8:
    case ActionSelectConsole9:
    case ActionSelectConsole10:
    case ActionSelectConsole11:
    case ActionSelectConsole12:
        if (self->sel_handler != 0) {
            unsigned console = lcode - ActionSelectConsole1;
            assert (console < 12);
            (*self->sel_handler)(console);
        }
        break;

    case ActionShutdown:
        if (self->shutdown_handler != 0) {
            (*self->shutdown_handler)();
        }
        break;

    default:
        kstr = km_get_string(&self->keymap, lcode, modifiers, buffer);
        if (kstr != 0) {
            if (self->pressed_handler != 0) {
                (*self->pressed_handler)(kstr);
            }
        }
        break;
    }
}

boolean kbd_start_request(usb_kbd_t *self)
{
    assert (self != 0);
    assert (self->ep != 0);
    assert (self->buffer != 0);

    usb_request(&self->urb, self->ep, self->buffer, BOOT_REPORT_SIZE, 0);
    usb_req_set_comp_cb(&self->urb, kbd_comp_cb, 0, self);
    return dw2_hc_submit_async_request(usb_func_get_host(&self->func), &self->urb, 0);
}

void kbd_comp_cb(usb_req_t *urb, void *param, void *ctx)
{
    usb_kbd_t *self = (usb_kbd_t *)ctx;
    assert (self != 0);
    assert (urb != 0);
    assert (&self->urb == urb);

    if ( usb_req_get_status(urb) != 0
      && usb_req_get_resultlen(urb) == BOOT_REPORT_SIZE) {
        if (self->status_handler != 0) {
            (*self->status_handler)(kbd_get_modifiers(self), self->buffer+2);
        } else {
            uint8_t pcode = kbd_get_kcode (self);

            if (pcode == self->pcode) {
                pcode = 0;
            } else {
                self->pcode = pcode;
            }

            if (pcode != 0) {
                kbd_generate_key_event(self, pcode);
#ifdef REPEAT_ENABLE
/*
                if (self->timer != 0) {
                    CancelKernelTimer(self->timer);
                }
                self->timer = StartKernelTimer (REPEAT_DELAY, kbd_timer_hdl, 0, self);
                assert (self->timer != 0);
*/
#endif
            } else if (self->timer != 0) {
                //CancelKernelTimer(self->timer);
                self->timer = 0;
            }
        }
    }
    _usb_request(&self->urb);
    kbd_start_request(self);
}

uint8_t kbd_get_modifiers(usb_kbd_t *self)
{
    assert (self != 0);
    return self->buffer[0];
}

uint8_t kbd_get_kcode(usb_kbd_t *self)
{
    assert (self != 0);

    for (unsigned i = 7; i >= 2; i--)
    {
        uint8_t kcode = self->buffer[i];
        if (kcode != 0) {
            return kcode;
        }
    }

    return 0;
}

#ifdef REPEAT_ENABLE

void kbd_timer_hdl(unsigned hTimer, void *param, void *ctx)
{
    usb_kbd_t *self = (usb_kbd_t *) ctx;
    assert (self != 0);

    assert (hTimer == self->timer);

    if (self->pcode != 0)
    {
        kbd_generate_key_event (self, self->pcode);

        //self->timer = StartKernelTimer (REPEAT_RATE, kbd_timer_hdl, 0, self);
        assert (self->timer != 0);
    }
}

#endif
