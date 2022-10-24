//
// devicenameservice.h
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
#ifndef INC_USB_DEVICENAMESERVICE_H
#define INC_USB_DEVICENAMESERVICE_H

#include "types.h"

typedef struct dev_info {
    struct dev_info *next;
    char            *name;
    void            *dev;
    boolean          blkdev;
} dev_info_t;

typedef struct device_ns {
    dev_info_t  *list;
} dev_ns_t;

void dev_name_service(dev_ns_t *self);
void _device_name_service(dev_ns_t *self);

void dev_name_service_add_dev(dev_ns_t *self, const char *name, void *dev, boolean blkdev);

void *dev_name_service_get_dev(dev_ns_t *self, const char *name, boolean blkdev);

dev_ns_t *dev_name_service_get(void);

#endif
