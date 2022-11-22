//
// dev_name_service_.c
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
#include "usb/devicenameservice.h"
#include "types.h"
#include "kmalloc.h"
#include "console.h"
#include "string.h"

static dev_ns_t *DEV_NS = 0;

void dev_name_service(dev_ns_t *self)
{
    self->list = 0;
    DEV_NS = self;
}

void _dev_name_service(dev_ns_t *self)
{
    while (self->list != 0) {
        dev_info_t *next = self->list->next;
        kmfree(self->list->name);
        self->list->name = 0;
        self->list->dev = 0;
        kmfree(self->list);
        self->list = next;
    }
    DEV_NS = 0;
}

void dev_name_service_add_dev(dev_ns_t *self, const char *name, void *dev, boolean blkdev)
{
    dev_info_t *info = (dev_info_t *) kmalloc(sizeof(dev_info_t));
    info->name = (char *) kmalloc(strlen(name)+1);
    safestrcpy(info->name, name, strlen(name)+1);
    info->dev = dev;
    info->blkdev = blkdev;
    info->next = self->list;
    self->list = info;
}

void *dev_name_service_get_dev(dev_ns_t *self, const char *name, boolean blkdev)
{
    dev_info_t *info = self->list;
    while (info != 0) {
        if (strcmp (name, info->name) == 0 && info->blkdev == blkdev) {
            return info->dev;
        }
        info = info->next;
    }

    return 0;
}

dev_ns_t *dev_name_service_get(void)
{
    return DEV_NS;
}
