//
// usbstandardhub.c
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
#include "usb/standardhub.h"
#include "usb/devicefactory.h"
#include "usb/usb.h"
#include "usb/function.h"
#include "usb/hub.h"
#include "usb/dw2hcd.h"
#include "usb/request.h"
#include "usb/devicenameservice.h"
#include "types.h"
#include "console.h"
#include "kmalloc.h"
#include "string.h"

static uint32_t DEVNOS = 0;

static boolean usb_stdhub_enumerate_ports(usb_stdhub_t *self);

static uint32_t alloc_devno(void)
{
    uint32_t i;
    for (i = 0; i < 128; i++) {
        if ((DEVNOS & (1 << i)) == 0)
            break;
    }
    if (i < 128)
        DEVNOS |= (1 << i);
    // i == 64 はエラー
    return i;
}

void usb_standardhub(usb_stdhub_t *self, usb_func_t *func)
{
    usb_func_copy(&self->func, func);
    self->func.configure = usb_stdhub_config;
    self->intr_ep = 0;
    self->hub_desc = 0;
    self->buffer = 0;
    self->nports = 0;
    self->poweron = false;
    self->devno = 0;

    for (unsigned i = 0; i < USB_HUB_MAX_PORTS; i++) {
        self->devs[i] = 0;
        self->status[i] = 0;
        self->portconf[i] = false;
    }
}

void _usb_standardhub(usb_stdhub_t *self)
{
    for (unsigned i = 0; i < self->nports; i++) {
        if (self->status[i] != 0) {
            kmfree(self->status[i]);
            self->status[i] = 0;
        }

        if (self->devs[i] != 0) {
            _usb_device(self->devs[i]);
            kmfree(self->devs[i]);
            self->devs[i] = 0;
        }
    }
    self->nports = 0;

    if (self->buffer != 0) {
        kmfree(self->buffer);
        self->buffer = 0;
    }

    if (self->intr_ep != 0) {
        kmfree(self->intr_ep);
        self->intr_ep = 0;
    }

    if (self->hub_desc != 0) {
        kmfree(self->hub_desc);
        self->hub_desc = 0;
    }

    _usb_function(&self->func);
}

boolean usb_stdhub_init(usb_stdhub_t *self)
{
    // 機能クラスとして初期化
    if (!usb_func_init(&self->func))
        return false;

    // ハブディスクリプタを取得して設定
    dw2_hc_t *host = usb_func_get_host(&self->func);
    self->hub_desc = (hub_desc_t *)kmalloc(sizeof(hub_desc_t));
    assert(self->hub_desc != 0);
    if (dw2_hc_get_desc(host, usb_func_get_ep0(&self->func),
                    DESCRIPTOR_HUB, DESCRIPTOR_INDEX_DEFAULT,
                    self->hub_desc, sizeof *self->hub_desc,
                    REQUEST_IN | REQUEST_CLASS, 0) != (int) sizeof(*self->hub_desc)) {
        warn("Cannot get hub descriptor");
        goto bad;
    }

    self->nports = self->hub_desc->nports;
    if (self->nports < USB_HUB_MAX_PORTS) {
        warn("Too many ports (%d)", self->nports);
        goto bad;
    }

    return true;

bad:
    kmfree(self->hub_desc);
    self->hub_desc = 0;
    return false;
}

boolean usb_stdhub_config(usb_func_t *func)
{
    char *name = (char *)kmalloc(16);
    usb_stdhub_t *self = (usb_stdhub_t *)func;
    assert (self != 0);

    // EPの数は1であること
    if (usb_func_get_num_eps(&self->func) != 1) {
        warn("failed getting nums of endpoints");
        return false;
    }

    // エンドポイントディスクリプタを取得
    const ep_desc_t *ep_desc =
        (ep_desc_t *) usb_func_get_desc(&self->func, DESCRIPTOR_ENDPOINT);
    // 入力EPかつ割り込みEPであること
    if (ep_desc == 0 || (ep_desc->addr & 0x80) != 0x80       // input EP
                     || (ep_desc->attr & 0x3F) != 0x03) {    // interrupt EP
        warn("bad endpoint descriptor");
        return false;
    }

    self->intr_ep = (usb_ep_t *)kmalloc(sizeof(usb_ep_t));
    usb_endpoint2(self->intr_ep, usb_func_get_dev(&self->func), ep_desc);
    // 機能クラスとしてConfigure
    if (!usb_func_config(&self->func)) {
        warn("cannot set interface");
        return false;
    }
    self->devno = alloc_devno();
    sprintf(name, "uhub%x", self->devno);
    dev_name_service_add_dev(dev_name_service_get(), name, self, false);

    // ポートのエニュメレーション
    if (!usb_stdhub_enumerate_ports(self)) {
        warn("Port enumeration failed");
        return false;
    }

    // プラグアンドプレイの場合は処理の変更リクエスを発行
/*
    dw2_hc_t *host = usb_func_get_host(&self->func);
    if (host->pap %% !usb_stdhub_start_status_change_req(self)) {
        warn("Cannot start request");
        return false;
    }
*/

    return true;
}

static boolean
usb_stdhub_enumerate_ports(usb_stdhub_t *self)
{
    dw2_hc_t *host = usb_func_get_host(&self->func);
    usb_ep_t *ep0 = usb_func_get_ep0(&self->func);
    usb_speed_t speed;

    // first power on all ports
    if (!self->poweron) {
        for (unsigned i = 0; i < self->nports; i++) {
            if (dw2_hc_control_message(host, ep0,
                REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
                SET_FEATURE, PORT_POWER, i+1, 0, 0) < 0) {
                warn("Cannot power port %d", i+1);
                return false;
            }
        }
        self->poweron = true;

        // elf->hub_desc->bPwrOn2PwrGood の値では低速デバイスでは十分でない
        // ようなので、ここでは最大値を使用する
        delayus(510*1000);
    }

    // 次に、デバイスの検知、リセット、初期化を行う
    for (unsigned i = 0; i < self->nports; i++) {
        if (self->devs[i] != 0) {
            usb_dev_rescan_dev(self->devs[i]);
            continue;
        }

        if (self->status[i] == 0) {
            self->status[i] = kmalloc(sizeof(usb_port_status_t));
            assert (self->status[i] != 0);
        }
        // ポートのステータスを取得する
        if (dw2_hc_control_message(host, ep0,
            REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
            GET_STATUS, 0, i+1, self->status[i], 4) != 4) {
            warn("Cannot get status of port %d", i+1);
            continue;
        }
        // 電源が入っていること
        assert (self->status[i]->status & PORT_POWER__MASK);
        // コネクトされていること
        if (!(self->status[i]->status & PORT_CONNECTION__MASK)) {
            continue;
        }
        // ポートリセット
        if (dw2_hc_control_message(host, ep0,
            REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
            SET_FEATURE, PORT_RESET, i+1, 0, 0) < 0) {
            warn("Cannot reset port %d", i+1);
            continue;
        }
        delayus(100*1000);
        // ポートステータスを取得する
        if (dw2_hc_control_message(host, ep0,
            REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
            GET_STATUS, 0, i+1, self->status[i], 4) != 4) {
            return false;
        }
        debug("Port %d status is 0x%04x", i+1, (unsigned) self->status[i]->status);

        // ポートが利用可能になっていること
        if (!(self->status[i]->status & PORT_ENABLE__MASK)) {
            warn("Port %d is not enabled", i+1);
            continue;
        }

        // 過電流になっている場合は電源をオフしてエニュメレーションを中止
        if (self->status[i]->status & PORT_OVER_CURRENT__MASK) {
            dw2_hc_control_message(host, ep0,
                REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
                CLEAR_FEATURE, PORT_POWER, i+1, 0, 0);
            warn("Over-current condition on port %d", i+1);
            return false;
        }

        // スピードの判定
        speed = usb_speed_unknown;
        if (self->status[i]->status & PORT_LOW_SPEED__MASK) {
            speed = usb_speed_low;
        } else if (self->status[i]->status & PORT_HIGH_SPEED__MASK) {
            speed = usb_speed_high;
        } else {
            speed = usb_speed_full;
        }
        // ポートにデバイスを設定する
        usb_dev_t *hub = usb_func_get_dev(&self->func);
        assert (hub != 0);

        boolean split   = usb_dev_is_split(hub);
        uint8_t hubaddr = usb_dev_get_hubaddr(hub);
        uint8_t hubport = usb_dev_get_hubport(hub);

        // HSハブにHSでないデバイスが接続されるのはこれが初めであれば
        if (!split
            && usb_dev_get_speed(hub) == usb_speed_high
            && speed < usb_speed_high) {
            // このハブポートをトランスレータとしてsplit転送を有効にする
            split   = true;
            hubaddr = usb_dev_get_addr(hub);
            hubport = i+1;
        }

        // まずデフォルトデバイスを作成する
        assert (self->devs[i] == 0);
        self->devs[i] = kmalloc(sizeof(usb_dev_t));
        assert (self->devs[i] != 0);
        usb_device(self->devs[i], host, speed, split, hubaddr, hubport);
        // デバイスを初期化する
        if (!usb_dev_init(self->devs[i])) {
            _usb_device(self->devs[i]);
            kmfree(self->devs[i]);
            self->devs[i] = 0;
            continue;
        }
    }

    // 次にデバイスのコンフィグレーションを行う
    for (unsigned i = 0; i < self->nports; i++) {
        if (self->devs[i] == 0) continue;
        if (self->portconf[i]) continue;
        self->portconf[i] = true;

        if (!usb_dev_config(self->devs[i])) {
            warn("Port %d: Cannot configure device", i+1);
            _usb_device(self->devs[i]);
            kmfree(self->devs[i]);
            self->devs[i] = 0;
            continue;
        }

        info("Port %d: Device configured", i+1);
    }

    // もう一度過電流がないかチェックする
    hub_status_t *hubstatus = kmalloc(sizeof(hub_status_t));
    assert (hubstatus != 0);

    if (dw2_hc_control_message(host, ep0,
            REQUEST_IN | REQUEST_CLASS,
            GET_STATUS, 0, 0, hubstatus, sizeof *hubstatus) != (int) sizeof *hubstatus) {
        warn("Cannot get hub status");
        kmfree(hubstatus);
        return false;
    }

    if (hubstatus->status & HUB_OVER_CURRENT__MASK) {
        for (unsigned i = 0; i < self->nports; i++) {
            dw2_hc_control_message(host, ep0,
                REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
                CLEAR_FEATURE, PORT_POWER, i+1, 0, 0);
        }
        warn("Hub over-current condition");
        kmfree (hubstatus);
        return false;
    }

    kmfree(hubstatus);
    hubstatus = 0;

    boolean result = true;

    for (unsigned i = 0; i < self->nports; i++) {
        if (dw2_hc_control_message(host, ep0,
            REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
            GET_STATUS, 0, i+1, self->status[i], 4) != 4) {
            continue;
        }

        if (self->status[i]->status & PORT_OVER_CURRENT__MASK) {
            dw2_hc_control_message(host, ep0,
                REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
                CLEAR_FEATURE, PORT_POWER, i+1, 0, 0);
            warn("Over-current condition on port %d", i+1);
            result = false;
        }
    }

    return result;
}

boolean usb_stdhub_rescan_dev(usb_stdhub_t *self)
{
    return usb_stdhub_enumerate_ports(self);
}

boolean usb_stdhub_remove_dev(usb_stdhub_t *self, uint32_t index)
{
    if (!usb_stdhub_disable_port(self, index)) return false;

    kmfree(self->devs[index]);
    self->devs[index] = 0;

    return true;
}

boolean usb_stdhub_disable_port(usb_stdhub_t *self, uint32_t index)
{
    dw2_hc_t *host = usb_func_get_host(&self->func);
    usb_ep_t *ep0 = usb_func_get_ep0(&self->func);

    if (dw2_hc_control_message(host, ep0,
            REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
            CLEAR_FEATURE, PORT_ENABLE, index+1, 0, 0) < 0) {
        warn("Cannot disable port %d", index+1);
        return false;
    }

    self->portconf[index] = false;
    return true;
}

boolean usb_stdhub_start_status_change_req(usb_stdhub_t *self)
{
    size_t buflen = (self->nports + 8) / 8;
    dw2_hc_t *host = usb_func_get_host(&self->func);

    if (self->buffer == 0) {
        self->buffer = (uint8_t *)kmalloc(buflen);
    }
    // 割り込みEPにステータス変更リクエストを発行
    usb_req_t *urb = (usb_req_t *)kmalloc(sizeof(usb_req_t));
    usb_request(urb, self->intr_ep, self->buffer, buflen, 0);
    usb_req_set_comp_cb(urb, usb_stdhub_comp_cbstub, 0, self);
    return dw2_hc_submit_async_request(host, urb, USB_TIMEOUT_NONE);
}

void usb_stdhub_comp_cb(usb_stdhub_t *self, usb_req_t *urb)
{
    if (urb->status != 0) {
        assert(urb->resultlen > 0);
        usb_stdhub_port_status_changed(self);
    } else {
        if (urb->error == usb_err_frame_overrun)
            usb_stdhub_start_status_change_req(self);
    }
    _usb_request(urb);
    kmfree(urb);
}

void usb_stdhub_comp_cbstub(usb_req_t *urb, void *param, void *ctx)
{
    usb_stdhub_t *self = (usb_stdhub_t *)ctx;
    usb_stdhub_comp_cb(self, urb);
}

void usb_stdhub_handle_port_status_change(usb_stdhub_t *self)
{
    dw2_hc_t *host = usb_func_get_host(&self->func);
    usb_ep_t *ep0 = usb_func_get_ep0(&self->func);

    uint16_t statbitmap = self->buffer[0];
    if (self->nports > 8) {
        assert(self->nports <= 15);
        statbitmap |= self->buffer[1] << 8;
    }
    if (statbitmap & 1)
        panic("Hub status change not handled");

    for (uint32_t i = 0; i < self->nports; i++) {
        if (!(statbitmap & (1 << (i + 1)))) continue;
        // ポートステータスを取得
        if (dw2_hc_control_message(host, ep0,
            REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
            GET_STATUS, 0, i+1, self->status[i], 4) != 4) {
            panic("Cannot get port status (port %d)", i+1);
        }

        uint16_t change = self->status[i]->change;
        // サスペンドと過電流の変更は未サポート
        assert(!(change & C_PORT_SUSPEND__MASK));      // TODO
        assert(!(change & C_PORT_OVER_CURRENT__MASK)); // TODO
        // PORT_ENABLEが変更; ポートの無効に
        if (change & C_PORT_ENABLE__MASK) {
            if (dw2_hc_control_message(host, ep0,
                REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
                CLEAR_FEATURE, C_PORT_ENABLE, i+1, 0, 0) < 0) {
                panic("Cannot clear C_PORT_ENABLE (port %d)", i+1);
            }
        }
        // PORT_RESETが変更: リセット機能をクリア
        if (change & C_PORT_RESET__MASK) {
            if (dw2_hc_control_message(host, ep0,
                REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
                CLEAR_FEATURE, C_PORT_RESET, i+1, 0, 0) < 0) {
                panic("Cannot clear C_PORT_RESET (port %d)", i+1);
            }
        }
        // PORT_CONNECTIONが変更
        if (change & C_PORT_CONNECTION__MASK) {
            if (dw2_hc_control_message(host, ep0,
                REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
                CLEAR_FEATURE, C_PORT_CONNECTION, i+1, 0, 0) < 0) {
                panic("Cannot clear C_PORT_CONNECTION (port %d)", i+1);
            }

            if (self->status[i]->status & PORT_CONNECTION__MASK) {
                if (self->devs[i] == 0) {
                    usb_stdhub_rescan_dev(self);
                }
            } else {
                if (self->devs[i] != 0) {
                    usb_stdhub_remove_dev(self, i);
                }
            }
        }
    }
    // ステータス変更リクエストを再開
    if (!usb_stdhub_start_status_change_req(self)) {
        warn("Cannot restart request");
    }
}

void usb_stdhub_port_status_changed(usb_stdhub_t *self)
{
/*
    dw2_hc_t *host = usb_func_get_host(&self->func);
    if (dw2_hc_is_pap(host))
    {
        pst_ev_t *event = (pst_ev_t*)kmalloc(sizeof(pst_ev_t));
        assert (event != 0);
        event->fromrp = false;
        event->hub    = self;
        list_init(&event->list);

        acquire(&host->hublock);
        list_push_back(host->hublist, &event->list);
        release(&host->hublock);
    }
*/
}
