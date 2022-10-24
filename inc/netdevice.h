//
// netdevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef INC_NETDEVICE_H
#define INC_NETDEVICE_H

#include "types.h"

#define FRAME_BUFFER_SIZE   1600
#define MAX_NET_DEVICES     5

/// @brief ネットデバイスタイプ
typedef enum net_device_type {
    net_dev_ethernet,
    net_dev_wlan,
    net_dev_any,
    net_dev_unknown
} net_dev_type_t;

/// @brief ネットデバイススピード
typedef enum net_device_speed {
    net_dev_speed_10half,
    net_dev_speed_10full,
    net_dev_speed_100half,
    net_dev_speed_100full,
    net_dev_speed_1000half,
    net_dev_speed_1000full,
    net_dev_speed_unknown
} net_speed_t;

struct net_device;
typedef struct net_device net_dev_t;

void netdev_add_net_dev(net_dev_t *dev);
const char *netdev_get_speedstr(net_speed_t speed);
net_dev_t *netdev_get_dev(unsigned num);
net_dev_t *netdev_get_dev_type(net_dev_type_t type);

#endif
