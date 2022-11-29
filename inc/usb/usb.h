//
// Form usb.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#ifndef INC_USB_USB_H
#define INC_USB_USB_H

#include "types.h"

// プラグアンドプレイを無効にするので、当面、これも無効にする
#undef USE_USB_SOF_INTR

typedef enum usb_pid {
    usb_pid_setup,
    usb_pid_data0,
    usb_pid_data1,
    //usb_pid_data2,
    //usb_pid_mdata
} usb_pid_t;

// Device Addresses
#define USB_DEFAULT_ADDRESS             0
#define USB_FIRST_DEDICATED_ADDRESS     1
#define USB_MAX_ADDRESS                 63    // normally 127

typedef enum usb_speed {
    usb_speed_low,
    usb_speed_full,
    usb_speed_high,
    usb_speed_super,
    usb_speed_unknown
} usb_speed_t;

typedef enum usb_error {
    usb_err_stall,
    usb_err_transaction,
    usb_err_babble,
    usb_err_frame_overrun,
    usb_err_data_toggle,
    usb_err_host_bus,
    usb_err_split,
    usb_err_timeout,
    usb_err_aborted,
    usb_err_unknown
} usb_err_t;

/// @brief セットアップデータ（リクエスト構造体の共通部分: 8バイト）を表す構造体
typedef struct setup_data {
    unsigned char       reqtype;
    unsigned char       req;
    unsigned short      value;
    unsigned short      index;
    unsigned short      length;
    // Data follows
} PACKED setup_data_t;

// Request Types
#define REQUEST_OUT             0
#define REQUEST_IN              0x80

#define REQUEST_CLASS           0x20
#define REQUEST_VENDOR          0x40

#define REQUEST_TO_DEVICE       0
#define REQUEST_TO_INTERFACE    1
#define REQUEST_TO_ENDPOINT     2
#define REQUEST_TO_OTHER        3

// Standard Request Codes
#define GET_STATUS              0
#define CLEAR_FEATURE           1
#define SET_FEATURE             3
#define SET_ADDRESS             5
#define GET_DESCRIPTOR          6
#define SET_CONFIGURATION       9
#define SET_INTERFACE           11

// Standard Feature Selectors
#define ENDPOINT_HALT           0

// Descriptor Types
#define DESCRIPTOR_DEVICE           1
#define DESCRIPTOR_CONFIGURATION    2
#define DESCRIPTOR_STRING           3
#define DESCRIPTOR_INTERFACE        4
#define DESCRIPTOR_ENDPOINT         5

// Class-specific descriptors
#define DESCRIPTOR_CS_INTERFACE     36
#define DESCRIPTOR_CS_ENDPOINT      37

#define DESCRIPTOR_INDEX_DEFAULT        0
#define USB_DEFAULT_MAX_PACKET_SIZE     8

/// @brief デバイス・ディスクリプタ構造体
typedef struct // device_descriptor
{
    unsigned char   length;
    unsigned char   type;
    unsigned short  version;
    unsigned char   class;
    unsigned char   subclass;
    unsigned char   proto;
    unsigned char   xsize0;
    unsigned short  vendorid;
    unsigned short  productid;
    unsigned short  device;
    unsigned char   manufact;
    unsigned char   product;
    unsigned char   serialno;
    unsigned char   nconfigs;
} PACKED dev_desc_t;

/// @brief コンフィギュレーション・ディスクリプタ構造体
typedef struct // configuration_escriptor
{
    unsigned char   length;
    unsigned char   type;
    unsigned short  total;
    unsigned char   interfaces;
    unsigned char   value;
    unsigned char   config;
    unsigned char   attr;
    unsigned char   xpower;
} PACKED cfg_desc_t;

/// @brief インタフェース・ディスクリプタ構造体
typedef struct // interface_descriptor
{
    unsigned char    length;
    unsigned char    type;
    unsigned char    num;
    unsigned char    alt;
    unsigned char    neps;
    unsigned char    class;
    unsigned char    subclass;
    unsigned char    proto;
    unsigned char    interface;
} PACKED if_desc_t;

/// @brief エンドポイント・ディスクリプタ構造体
typedef struct // endpoint_descriptor
{
    unsigned char   length;
    unsigned char   type;
    unsigned char   addr;
    unsigned char   attr;
    unsigned short  xsize;
    unsigned char   interval;
} PACKED ep_desc_t;

/// @brief USBディスクリプタを表すUNION
typedef union
{
    struct {
        unsigned char   length;
        unsigned char   type;
    } header;

    cfg_desc_t          config;
    if_desc_t           interface;
    ep_desc_t           endpoint;
} PACKED usb_desc_t;

/// @brief ストリング・ディスクリプタ構造体
typedef struct // string_descriptor
{
    unsigned char    len;
    unsigned char    type;
    unsigned short   string[0];
} PACKED str_desc_t;

#endif
