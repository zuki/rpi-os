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


#define MAX_CONFIG_DESC_SIZE    512        // best guess

typedef struct configuration_header {
    cfg_desc_t  config;
    if_desc_t   interface;
} config_header_t;


static uint8_t next_addr = USB_FIRST_DEDICATED_ADDRESS;

void usb_device(usb_dev_t *self, dw2_hc_t *host, usb_speed_t speed,
        boolean split, uint8_t hubaddr, uint8_t hubport)
{
    self->host = host;
    self->addr = USB_DEFAULT_ADDRESS;
    self->speed = speed;
    self->ep0 = 0;
    self->split = split;
    self->hubaddr = hubaddr;
    self->hubport = hubport;
    self->tt_hub = 0;           // TOFO
    self->dev_desc = 0;
    self->cfg_desc = 0;
    self->cfg_parser = 0;
    self->ep0 = (usb_ep_t *) kmalloc(sizeof(usb_ep_t));
    assert (self->ep0 != 0);
    usb_endpoint(self->ep0, self);
    assert (hubport >= 1);
    usb_string(self->manufact, self);
    usb_string(self->product, self);

    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        self->usb_func[i] = 0;
    }
}

void _usb_device(usb_dev_t *self)
{
    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        if (self->usb_func[i] != 0) {
            _usb_function(self->usb_func[i]);
            kmfree(self->usb_func[i]);
            self->usb_func[i] = 0;
        }
    }

    if (self->cfg_parser != 0) {
        _cfg_paser(self->cfg_parser);
        kmfree(self->cfg_parser);
        self->cfg_parser = 0;
    }

    if (self->cfg_desc != 0) {
        kmfree(self->cfg_desc);
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

    _usb_string(self->product);
    _usb_string(self->manufact);
}

boolean usb_dev_init(usb_dev_t *self)
{
    assert (self->dev_desc == 0);
    self->dev_desc = (dev_desc_t *) kmalloc(sizeof (dev_desc_t));
    assert(sizeof *self->dev_desc >= USB_DEFAULT_MAX_PACKET_SIZE);
    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_DEVICE, DESCRIPTOR_INDEX_DEFAULT,
                    self->dev_desc, USB_DEFAULT_MAX_PACKET_SIZE, REQUEST_IN, 0)
        != USB_DEFAULT_MAX_PACKET_SIZE) {
        error("Cannot get device descriptor (short)");
        kmfree(self->dev_desc);
        self->dev_desc = 0;
        return false;
    }

    if (self->dev_desc->length != sizeof *self->dev_desc
     || self->dev_desc->type   != DESCRIPTOR_DEVICE) {
        error("Invalid device descriptor");
        kmfree(self->dev_desc);
        self->dev_desc = 0;
        return false;
    }

    usb_ep_set_max_packet_size(self->ep0, self->dev_desc->xsize0);

    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_DEVICE, DESCRIPTOR_INDEX_DEFAULT,
                    self->dev_desc, sizeof *self->dev_desc, REQUEST_IN, 0)
        != (int) sizeof *self->dev_desc) {
        error("Cannot get device descriptor");
        kmfree(self->dev_desc);
        self->dev_desc = 0;
        return false;
    }

    // DebugHexdump (self->dev_desc, sizeof *self->dev_desc, "usbdevice");

    uint8_t addr = next_addr++;
    if (addr > USB_MAX_ADDRESS) {
        error("Too many devices");
        return false;
    }

    if (!dw2_hc_set_addr(self->host, self->ep0, addr)) {
        error("Cannot set address %d", addr);
        return false;
    }

    usb_dev_set_addr(self, addr);

    if (self->dev_desc->manufact != 0 || self->dev_desc->product != 0) {
        uint16_t langid = usb_string_get_langid(self->manufact);

        if (self->dev_desc->manufact != 0) {
            usb_string_get_from_desc(self->manufact, self->dev_desc->manufact, langid) ;
        }

        if (self->dev_desc->product != 0) {
            usb_string_get_from_desc(self->product, self->dev_desc->product, langid);
        }
    }

    assert (self->cfg_desc == 0);
    self->cfg_desc = (cfg_desc_t *)kmalloc(sizeof (cfg_desc_t));
    assert (self->cfg_desc != 0);

    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_CONFIGURATION, DESCRIPTOR_INDEX_DEFAULT,
                    self->cfg_desc, sizeof *self->cfg_desc, REQUEST_IN, 0)
        != (int) sizeof *self->cfg_desc) {
        error("Cannot get configuration descriptor (short)");
        kmfree(self->cfg_desc);
        self->cfg_desc = 0;
        return false;
    }

    if (self->cfg_desc->length != sizeof *self->cfg_desc
     || self->cfg_desc->type   != DESCRIPTOR_CONFIGURATION
     || self->cfg_desc->total  >  MAX_CONFIG_DESC_SIZE) {
        error("Invalid configuration descriptor");
        kmfree(self->cfg_desc);
        self->cfg_desc = 0;
        return false;
    }

    unsigned total = self->cfg_desc->total;

    kmfree(self->cfg_desc);
    self->cfg_desc = (cfg_desc_t *)kmalloc(total);
    assert (self->cfg_desc != 0);

    if (dw2_hc_get_desc(self->host, self->ep0,
                    DESCRIPTOR_CONFIGURATION, DESCRIPTOR_INDEX_DEFAULT,
                    self->cfg_desc, total, REQUEST_IN, 0)
        != (int) total) {
        error("Cannot get configuration descriptor");
        kmfree(self->cfg_desc);
        self->cfg_desc = 0;
        return false;
    }

    // DebugHexdump (self->cfg_desc, total, FromDevice);

    assert (self->cfg_parser == 0);
    self->cfg_parser = (cfg_parser_t *)kmalloc (sizeof (cfg_parser_t));
    assert (self->cfg_parser != 0);
    cfg_parser(self->cfg_parser, self->cfg_desc, total);

    if (!cfg_parser_is_valid(self->cfg_parser)) {
        cfg_parser_error(self->cfg_parser, "usbdevice");
        return false;
    }

    char *names = (char *)kmalloc(128);
    names = usb_dev_get_names(self);
    assert (names != 0);
    info("Device %s found", names);
    kmfree(names);

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

    return true;
}

boolean usb_dev_config(usb_dev_t *self)
{
    if (self->cfg_desc == 0) {      // 初期化されていない
        warn("cfg_desc == 0");
        return false;
    }

    if (!dw2_hc_set_config(self->host, self->ep0, self->cfg_desc->value)) {
        error("Cannot set configuration (%d)", (int)self->cfg_desc->value);
        return false;
    }

    boolean result = false;

    for (unsigned i = 0; i < USBDEV_MAX_FUNCTIONS; i++) {
        if (self->usb_func[i] != 0) {
            if (!usb_func_config(self->usb_func[i])) {
                warn("Cannot configure device %d", i);
                _usb_function(self->usb_func[i]);
                kmfree(self->usb_func[i]);
                self->usb_func[i] = 0;
            } else {
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
