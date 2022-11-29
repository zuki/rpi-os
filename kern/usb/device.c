//
// usbdevice.c
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
#include "usb/device.h"
#include "usb/dw2hcd.h"
#include "usb/endpoint.h"
#include "usb/devicefactory.h"
#include "types.h"
#include "console.h"
#include "kmalloc.h"
#include "string.h"
#include "debug.h"
#include "mm.h"


#define MAX_CONFIG_DESC_SIZE    512        // best guess

typedef struct configuration_header {
    cfg_desc_t  config;
    if_desc_t   interface;
} config_header_t;


static uint8_t next_addr = USB_FIRST_DEDICATED_ADDRESS;

void usb_device(usb_dev_t *self, dw2_hc_t *host, usb_speed_t speed,
        dw2_rport_t *rport)
{
    self->host = host;
    self->rport = rport;
    self->hub = 0;
    self->pindex = 0;
    self->addr = USB_DEFAULT_ADDRESS;
    self->speed = speed;
    self->ep0 = 0;
    self->split = false;
    self->hubaddr = 0;
    self->hubport = 1;
    self->tt_hub = 0;
    self->dev_desc = 0;
    self->cfg_desc = 0;
    self->cfg_parser = 0;
    self->ep0 = (usb_ep_t *) kmalloc(sizeof(usb_ep_t));
    assert (self->ep0 != 0);
    usb_endpoint(self->ep0, self);
    self->manufact = 0;
    self->product= 0;

    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        self->usb_func[i] = 0;
    }
}

void usb_device2(usb_dev_t *self, dw2_hc_t *host, usb_speed_t speed,
        usb_stdhub_t *hub, unsigned pindex)
{
    self->host = host;
    self->rport = 0;
    self->hub = hub;
    self->pindex = pindex;
    self->addr = USB_DEFAULT_ADDRESS;
    self->speed = speed;
    self->ep0 = 0;
    self->split = false;
    self->dev_desc = 0;
    self->cfg_desc = 0;
    self->cfg_parser = 0;

    usb_dev_t *hubdev = hub->func.dev;
    assert(hubdev != 0);

    self->split = hubdev->split;
    self->hubaddr = hubdev->hubaddr;
    self->hubport = hubdev->hubport;
    self->tt_hub = hubdev->tt_hub;

    // これはハブチェーンの中の非HSデバイスを持つ最初のHSハブか
    if (!self->split && hubdev->speed == usb_speed_high && self->speed < usb_speed_high) {
        // そうであればこのハブはスプリット転送を有効にする
        self->split = true;
        self->hubaddr = hubdev->addr;
        self->hubport = self->pindex + 1;
        self->tt_hub = hubdev;
    }

    self->ep0 = (usb_ep_t *) kmalloc(sizeof(usb_ep_t));
    assert (self->ep0 != 0);
    usb_endpoint(self->ep0, self);

    self->manufact = 0;
    self->product = 0;

    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        self->usb_func[i] = 0;
    }
}

void _usb_device(usb_dev_t *self)
{
    if (self->host != 0)
        dw2_hc_cancel_device_transaction(self->host, self);

    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        if (self->usb_func[i] != 0) {
            _usb_function(self->usb_func[i]);
            kmfree(self->usb_func[i]);
            self->usb_func[i] = 0;
        }
    }

    if (self->dev_desc) {
        char *names = usb_dev_get_names(self);
        info("Device %s removed", names);
        kmfree(names);
    }

    if (self->cfg_parser != 0) {
        _cfg_paser(self->cfg_parser);
        kmfree(self->cfg_parser);
        self->cfg_parser = 0;
    }

    if (self->cfg_desc != 0) {
        //kmfree(self->cfg_desc);
        kfree(self->cfg_desc);
        self->cfg_desc = 0;
    }

    if (self->dev_desc != 0)
    {
        kmfree(self->dev_desc);
        self->dev_desc = 0;
    }

    if (self->ep0 != 0) {
        _usb_endpoint(self->ep0);
        kmfree(self->ep0);
    }

    self->host = 0;

    if (self->product != 0) {
        _usb_string(self->product);
        kmfree(self->product);
        self->product = 0;
    }

    if (self->manufact != 0) {
        _usb_string(self->manufact);
        kmfree(self->manufact);
        self->manufact = 0;
    }
}

boolean usb_dev_init(usb_dev_t *self)
{
    trace("usb_dev_init start");
    assert (self->dev_desc == 0);
    // FIXME: 64 byte alignのメモリアロケータを作成する
    //self->dev_desc = (dev_desc_t *) kmalloc(sizeof (dev_desc_t));
    self->dev_desc = (dev_desc_t *) kalloc();
    assert(sizeof *self->dev_desc >= USB_DEFAULT_MAX_PACKET_SIZE);
    // 1.  8バイトデバイスディスクリプタを取得
    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_DEVICE, DESCRIPTOR_INDEX_DEFAULT,
                    self->dev_desc, USB_DEFAULT_MAX_PACKET_SIZE, REQUEST_IN, 0)
        != USB_DEFAULT_MAX_PACKET_SIZE) {
        error("Cannot get device descriptor (short)");
        //kmfree(self->dev_desc);
        kfree(self->dev_desc);
        self->dev_desc = 0;
        return false;
    }

    if (self->dev_desc->length != sizeof *self->dev_desc
     || self->dev_desc->type   != DESCRIPTOR_DEVICE) {
        error("Invalid device descriptor");
        //kmfree(self->dev_desc);
        kfree(self->dev_desc);
        self->dev_desc = 0;
        return false;
    }
    DataMemBarrier();
    //debug_struct("dev_desc(8)", self->dev_desc, 8);

    // 2. 詳細なデバイスディスクリプタを取得
    self->ep0->xsize = self->dev_desc->xsize0;

    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_DEVICE, DESCRIPTOR_INDEX_DEFAULT,
                    self->dev_desc, sizeof *self->dev_desc, REQUEST_IN, 0)
        != (int) sizeof *self->dev_desc) {
        error("Cannot get device descriptor");
        kmfree(self->dev_desc);
        self->dev_desc = 0;
        return false;
    }

    DataMemBarrier();
    debug_struct("dev_desc", self->dev_desc, sizeof *self->dev_desc);

    // 3. アドレスを採番
    uint8_t addr = next_addr++;
    if (addr > USB_MAX_ADDRESS) {
        error("Too many devices");
        return false;
    }
    trace("3: addr=%d", addr);
    // 4. デバイスにアドレスをセット
    if (!dw2_hc_set_addr(self->host, self->ep0, addr)) {
        error("Cannot set address %d", addr);
        return false;
    }
    self->addr = addr;
    trace("4");
    // 5. コンフィグレーションディスクリプタ（先頭のconfig_descのみ）を仮設定
    assert (self->cfg_desc == 0);
    // FIXME
    //self->cfg_desc = (cfg_desc_t *)kmalloc(sizeof (cfg_desc_t));
    self->cfg_desc = (cfg_desc_t *)kalloc();
    assert (self->cfg_desc != 0);
    trace("cfg_desc[1]=0x%p", self->cfg_desc);
    // 5.1 コンフィグレーションインデックスの設定
    uint8_t cindex = DESCRIPTOR_INDEX_DEFAULT;
    // 5.2 QEMUのEthernetデバイスの場合は、特別なサポート
    if (self->dev_desc->vendorid == 0x0525      // NetChip
     || self->dev_desc->productid == 0xA4A2) {  // Ethernet/RNDIS Gadget
        cindex++;
    }
    trace("5.2: cindex=%d", cindex);
    // 5.3 コンフィグレーションディスクリプタを取得
    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_CONFIGURATION, cindex,
                    self->cfg_desc, sizeof *self->cfg_desc, REQUEST_IN, 0)
        != (int) sizeof *self->cfg_desc) {
        error("Cannot get configuration descriptor (short)");
        //kmfree(self->cfg_desc);
        kfree(self->cfg_desc);
        self->cfg_desc = 0;
        return false;
    }
    trace("5.3");;
    // 5.4 コンフィグレーションディスクリプタが不正
    if (self->cfg_desc->length != sizeof *self->cfg_desc
     || self->cfg_desc->type   != DESCRIPTOR_CONFIGURATION
     || self->cfg_desc->total  >  MAX_CONFIG_DESC_SIZE) {
        error("Invalid configuration descriptor");
        //kmfree(self->cfg_desc);
        kfree(self->cfg_desc);
        self->cfg_desc = 0;
        return false;
    }
    DataMemBarrier();
    //debug_struct("config_desc(8)", self->cfg_desc, sizeof(cfg_desc_t));

    trace("5.4");;
    // 6. 正式版のコンフィグレーションディスクリプタを取得
    unsigned total = self->cfg_desc->total;
    // 6.1 8バイト版コンフィグレーションディスクリプタを削除
    //kmfree(self->cfg_desc);
    memset(self->cfg_desc, 0, total);
    DataMemBarrier();

    // 6.2 正式なコンフィグレーションディスクリプタ用のスペースを確保
    //self->cfg_desc = (cfg_desc_t *)kmalloc(total);
    assert (self->cfg_desc != 0);
    trace("cfg_desc[2]=0x%p", self->cfg_desc);
    // 6.3 正式なコンフィグレーションディスクリプタを取得
    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_CONFIGURATION, cindex,
                    self->cfg_desc, total, REQUEST_IN, 0)
        != (int) total) {
        error("Cannot get configuration descriptor");
        //kmfree(self->cfg_desc);
        kfree(self->cfg_desc);
        self->cfg_desc = 0;
        return false;
    }
    trace("6");
    //DataMemBarrier();
    debug_struct("config_desc", self->cfg_desc, (uint64_t)total);

    // 7. コンフィグレーションパーサを作成
    assert (self->cfg_parser == 0);
    self->cfg_parser = (cfg_parser_t *)kmalloc (sizeof (cfg_parser_t));
    assert (self->cfg_parser != 0);
    cfg_parser(self->cfg_parser, self->cfg_desc, total);
    if (!self->cfg_parser->valid) {
        cfg_parser_error(self->cfg_parser, "usbdevice");
        return false;
    }
    trace("7");
    // 8. デバイス名を表示
    char *names = (char *)kmalloc(128);
    names = usb_dev_get_names(self);
    assert (names != 0);
    info("Device %s found", names);
    kmfree(names);

    if (self->dev_desc->manufact != 0 || self->dev_desc->product != 0) {
        if (self->dev_desc->manufact != 0) {
            if (self->manufact == 0) {
                self->manufact = (usb_str_t *)kmalloc(sizeof(usb_str_t));
                usb_string(self->manufact, self);
            }
            usb_string_get_from_desc(self->manufact, self->dev_desc->manufact, usb_string_get_langid(self->manufact)) ;
        }

        if (self->dev_desc->product != 0) {
            if (self->product == 0) {
                self->product = (usb_str_t *)kmalloc(sizeof(usb_str_t));
                usb_string(self->product, self);
            }
            usb_string_get_from_desc(self->product, self->dev_desc->product, usb_string_get_langid(self->manufact));
        }
        info("Product: %s %s", self->manufact != 0 ? self->manufact->str : "",
                               self->product != 0 ? self->product->str : "");
    }
    trace("8");
    // 9. コンフィグレーションディスクリプタからインタフェースを取得する
    unsigned i = 0;
    uint8_t num = 0;
    if_desc_t *if_desc;
    while ((if_desc = (if_desc_t *) cfg_parser_get_desc(self->cfg_parser, DESCRIPTOR_INTERFACE)) != 0) {
        if (if_desc->num > num) {
            num = if_desc->num;
        }

        if (if_desc->num != num) {
            debug("Alternate setting %d ignored", (int) if_desc->alt);
            continue;
        }

        assert (self->cfg_parser != 0);
        assert (self->usb_func[i] == 0);
        self->usb_func[i] = (usb_func_t *)kmalloc(sizeof(usb_func_t));
        assert (self->usb_func[i] != 0);
        usb_function(self->usb_func[i], self, self->cfg_parser);

        usb_func_t *child = 0;

        if (i == 0) {
            child = usb_devfactory_get_device(self->usb_func[i], usb_dev_get_name(self, dev_name_vendor));
            if (child == 0) {
                child = usb_devfactory_get_device(self->usb_func[i], usb_dev_get_name(self, dev_name_device));
            }
        }

        if (child == 0) {
            // nameはusb_func_get_if_nameでkmalloc、usb_devfactory_get_deviceでkmfree
            char *name = usb_func_get_if_name(self->usb_func[i]);
            if (strncmp(name, "unknown", strlen(name)) != 0) {
                info("Interface %s found", name);
                child = usb_devfactory_get_device(self->usb_func[i], name);
            } else {
                kmfree(name);
            }
        }

        _usb_function(self->usb_func[i]);
        kmfree(self->usb_func[i]);
        self->usb_func[i] = 0;

        if (child == 0) {
            warn("Function is not supported");
            continue;
        }

        self->usb_func[i] = child;

        if (++i == USBDEV_MAX_FUNCTIONS) {
            warn("Too many functions per device");
            break;
        }
        num++;
    }

    if (i == 0) {
        warn("Device has no supported function");
        return false;
    }
    trace("9");
    return true;
}

boolean usb_dev_config(usb_dev_t *self)
{
    if (self->cfg_desc == 0) {      // 初期化されていない
        warn("cfg_desc == 0");
        return false;
    }

    if (!dw2_hc_set_config(self->host, self->ep0, self->cfg_desc->value)) {
        warn("Cannot set configuration (%d)", (int)self->cfg_desc->value);
        return false;
    }

    boolean result = false;

    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        if (self->usb_func[i] != 0) {
            if (!self->usb_func[i]->configure(self->usb_func[i])) {
                warn("Cannot configure device %d", i);
                _usb_function(self->usb_func[i]);
                kmfree(self->usb_func[i]);
                self->usb_func[i] = 0;
            } else {
                trace("%d is ok", i);
                result = true;
            }
        }
    }

    return result;
}

char *usb_dev_get_name(usb_dev_t *self, selector_t selector)
{
    assert (self != 0);
    char *name = (char *)kmalloc(128);
    assert (name != 0);

    switch (selector) {
    case dev_name_vendor:
        assert (self->dev_desc != 0);
        sprintf(name, "ven%x-%x",
                 (int) self->dev_desc->vendorid,
                 (int) self->dev_desc->productid);
        break;

    case dev_name_device:
        assert (self->dev_desc != 0);
        if (self->dev_desc->class == 0 || self->dev_desc->class == 0xFF) {
            goto unknown;
        }
        sprintf(name, "dev%x-%x-%x",
                 (int) self->dev_desc->class,
                 (int) self->dev_desc->subclass,
                 (int) self->dev_desc->proto);
        break;

    default:
        assert (0);
    unknown:
        safestrcpy(name, "unknown", 8);
        break;
    }

    return name;
}

char *usb_dev_get_names(usb_dev_t *self)
{
    assert (self != 0);

    char *names = kmalloc(512);
    assert (names != 0);

    for (unsigned selector = dev_name_vendor; selector < dev_name_unknown; selector++) {
        char *name = usb_dev_get_name(self, (selector_t)selector);
        assert (name != 0);

        if (strncmp(name, "unknown", strlen(name)) != 0) {
            if (strlen(names) > 0) {
                memmove(names+strlen(names), ", ", 3);
            }
            memmove(names+strlen(names), name, strlen(name)+1);
        }
        kmfree(name);
    }

    if (strlen(names) == 0) {
        safestrcpy(names, "unknown", strlen("unknown"));
    }

    return names;
}

uint8_t usb_dev_get_addr(usb_dev_t *self)
{
    return self->addr;
}

void usb_dev_set_addr(usb_dev_t *self, uint8_t addr)
{
    self->addr = addr;
}

usb_speed_t usb_dev_get_speed(usb_dev_t *self)
{
    return self->speed;
}

boolean usb_dev_is_split(usb_dev_t *self)
{
    return self->split;
}

uint8_t usb_dev_get_hubaddr(usb_dev_t *self)
{
    return self->hubaddr;
}

uint8_t usb_dev_get_hubport(usb_dev_t *self)
{
    return self->hubport;
}

usb_ep_t *usb_dev_get_endpoint0(usb_dev_t *self)
{
    return self->ep0;
}

dw2_hc_t *usb_device_get_host(usb_dev_t *self)
{
    return self->host;
}

const dev_desc_t *usb_dev_get_dev_desc(usb_dev_t *self)
{
    return self->dev_desc;
}

const cfg_desc_t *usb_dev_get_cfg_desc(usb_dev_t *self)
{
    return self->cfg_desc;
}

const usb_desc_t *usb_dev_get_desc(usb_dev_t *self, uint8_t type)
{
    return cfg_parser_get_desc(self->cfg_parser, type);
}

void usb_dev_cfg_error(usb_dev_t *self, const char *source)
{
    cfg_parser_error(self->cfg_parser, source);
}

void usb_device_set_addr(usb_dev_t *self, uint8_t addr)
{
    self->addr = addr;
    //debug("Device address set to %u", (unsigned) self->addr);
}

boolean usb_dev_rescan_dev(usb_dev_t *self)
{
    boolean result = false;

    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        if (self->usb_func[i] != 0) {
            if (usb_func_rescan_dev(self->usb_func[i])) {
                result = true;
            }
        }
    }

    return result;
}

boolean usb_dev_remove_dev(usb_dev_t *self)
{
    // no_op: xchideviceのみ
    return true;
}
