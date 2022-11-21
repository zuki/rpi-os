#include "usb/function.h"
#include "usb/device.h"
#include "usb/dw2hcd.h"
#include "usb/endpoint.h"
#include "usb/config_parser.h"
#include "types.h"
#include "console.h"
#include "kmalloc.h"
#include "string.h"

void usb_function(usb_func_t *self, usb_dev_t *dev, cfg_parser_t *parser)
{
    self->configure = 0;
    self->dev = dev;
    self->cfg_parser = (cfg_parser_t *)kmalloc(sizeof(cfg_parser_t));
    cfg_parser_copy(self->cfg_parser, parser);
    self->if_desc = (if_desc_t *)cfg_parser_get_curr_desc(self->cfg_parser);
}

void usb_func_copy(usb_func_t *self, usb_func_t *func)
{
    self->configure = func->configure;
    self->dev = func->dev;
    self->cfg_parser = (cfg_parser_t *)kmalloc(sizeof(cfg_parser_t));
    cfg_parser_copy(self->cfg_parser, func->cfg_parser);
    self->if_desc = (if_desc_t *)cfg_parser_get_curr_desc(self->cfg_parser);
}

void _usb_function(usb_func_t *self)
{
    self->if_desc = 0;
    _cfg_paser(self->cfg_parser);
    kmfree(self->cfg_parser);
    self->cfg_parser = 0;
    self->dev = 0;
    self->configure = 0;
}

boolean usb_func_init(usb_func_t *self)
{
    return true;
}

boolean usb_func_config(usb_func_t *self)
{
    assert(self->if_desc != 0);
    if (self->if_desc->alt != 0) {
        if (dw2_hc_control_message(self->dev->host,
            self->dev->ep0, REQUEST_OUT | REQUEST_TO_INTERFACE, SET_INTERFACE,
            self->if_desc->alt, self->if_desc->num, 0, 0) < 0) {
            return false;
        }
    }
    return true;
}

boolean usb_func_rescan_dev(usb_func_t *self)
{
    return false;
}

boolean usb_func_remove_dev(usb_func_t *self)
{
    return usb_dev_remove_dev(self->dev);
}

char *usb_func_get_if_name(usb_func_t *self)
{
    if_desc_t *desc = self->if_desc;
    char *name = (char *)kmalloc(64);

    if (desc != 0 && desc->class != 0x00 && desc->class != 0xff) {
        sprintf(name, "int%x-%x-%x", desc->class, desc->subclass, desc->proto);
    } else {
        memmove(name, "unknown", 8);
    }
    info("func name=%s", name);

    return name;
}

uint8_t usb_func_get_num_eps(usb_func_t *self)
{
    return self->if_desc->neps;
}

boolean usb_func_select_if(usb_func_t *self, uint8_t class, uint8_t subclass, uint8_t proto)
{
    do {
        if (self->if_desc->class    == class
         && self->if_desc->subclass == subclass
         && self->if_desc->proto    == proto) {
            return true;
        }
        // 次のインタフェースにスキップ
        usb_dev_get_desc(self->dev, DESCRIPTOR_INTERFACE);
    } while ((self->if_desc = (if_desc_t *)usb_func_get_desc(self, DESCRIPTOR_INTERFACE)) != 0);

    return false;
}

usb_dev_t *usb_func_get_dev(usb_func_t *self)
{
    return self->dev;
}

usb_ep_t *usb_func_get_ep0(usb_func_t *self)
{
    return self->dev->ep0;
}

dw2_hc_t *usb_func_get_host(usb_func_t *self)
{
    return self->dev->host;
}

const usb_desc_t *usb_func_get_desc(usb_func_t *self, uint8_t type)
{
    return cfg_parser_get_desc(self->cfg_parser, type);
}


uint8_t usb_func_get_if_num(usb_func_t *self)
{
    return self->if_desc->num;
}

uint8_t usb_func_get_if_class(usb_func_t *self)
{
    return self->if_desc->class;
}

uint8_t usb_func_get_if_subclass(usb_func_t *self)
{
    return self->if_desc->subclass;
}

uint8_t usb_func_get_if_proto(usb_func_t *self)
{
    return self->if_desc->proto;
}
