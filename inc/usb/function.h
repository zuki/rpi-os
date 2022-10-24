//
// usbfunction.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef INC_USB_FUNCTION_H
#define INC_USB_FUNCTION_H

#include "usb/config_parser.h"
#include "usb/usb.h"
#include "types.h"
#include "usb.h"

struct usb_dev;
struct dw2_hc;
struct usb_ep;

/// @brief デバイス機能クラスを表す構造体
typedef struct usb_function {
    boolean        (*configure)(struct usb_function *self);
    struct usb_dev  *dev;           ///< デバイス
    cfg_parser_t    *cfg_parser;    ///< コンフィグレーションパーサ
    if_desc_t       *if_desc;       ///< インタフェースディスクリプタ
} usb_func_t;

void usb_function(usb_func_t *self, struct usb_dev *dev, cfg_parser_t *parser);
void _usb_function(usb_func_t *self);
void usb_func_copy(usb_func_t *self, usb_func_t *func);

boolean usb_func_init(usb_func_t *self);
boolean usb_func_config(usb_func_t *self);
boolean usb_func_rescan_dev(usb_func_t *self);
boolean usb_func_remove_device(usb_func_t *self);
char *usb_func_get_if_name(usb_func_t *self);
uint8_t usb_func_get_num_eps(usb_func_t *self);
boolean usb_func_select_if(usb_func_t *self, uint8_t class, uint8_t subclass, uint8_t proto);

struct usb_dev *usb_func_get_dev(usb_func_t *self);
struct usb_ep *usb_func_get_ep0(usb_func_t *self);
struct dw2_hc *usb_func_get_host(usb_func_t *self);

const usb_desc_t *usb_func_get_desc(usb_func_t *self, uint8_t type);

uint8_t usb_func_get_if_num(usb_func_t *self);
uint8_t usb_func_get_if_class(usb_func_t *self);
uint8_t usb_func_get_if_subclass(usb_func_t *self);
uint8_t usb_func_get_if_proto(usb_func_t *self);

#endif
