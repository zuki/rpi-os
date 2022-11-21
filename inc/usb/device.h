//
// usb_dev_.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright(C) 2014-2020  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef INC_USB_DEVICE_H
#define INC_USB_DEVICE_H

#include "types.h"
#include "usb/usb.h"
#include "usb/config_parser.h"
#include "usb/function.h"
#include "usb/string.h"
#include "usb/dw2rootport.h"
#include "usb/standardhub.h"

#define USBDEV_MAX_FUNCTIONS    10

extern uint64_t NUM_POOL;        // デバイスアドレスプール

/// @brief デバイス名セレクタ
typedef enum { // 順序を変更しない
    dev_name_vendor,
    dev_name_device,
    dev_name_unknown
} selector_t;

struct dw2_hc;
struct dw2_rport;
struct standardhub;
struct usb_ep;
struct usb_dev;


/// @brief USBデバイスを表すクラス
typedef struct usb_dev {
    struct dw2_hc      *host;               ///< ホストコントローラ
    struct dw2_rport   *rport;              ///< このデバイスが接続されているルートポート
    struct standardhub *hub;           ///< このデバイスが接続されているハブ
    unsigned            pindex;             ///< このハブの0ベースのインデックス
    uint8_t             addr;               ///< アドレス
    usb_speed_t         speed;              ///< スピード
    struct usb_ep      *ep0;                ///< エンドポイント 0
    boolean             split;              ///< スプリット転送か
    uint8_t             hubaddr;            ///< ハブアドレス
    uint8_t             hubport;            ///< ハブポート番号
    struct usb_dev     *tt_hub;             ///< TTハブ(circle)

    dev_desc_t         *dev_desc;           ///< デバイスディスクリプタ
    cfg_desc_t         *cfg_desc;           ///< コンフィグレーションディスクリプタ

    cfg_parser_t       *cfg_parser;         ///< コンフィグレーションパーサ
    usb_str_t          *manufact;           ///< 製造者文字列(uspi)
    usb_str_t          *product;            ///< 製品名文字列(uspi)
    usb_func_t         *usb_func[USBDEV_MAX_FUNCTIONS]; ///< デバイスクラス
} usb_dev_t;

void usb_device(usb_dev_t *self, struct dw2_hc *host, usb_speed_t speed,
		struct dw2_rport *rport);
void usb_device2(usb_dev_t *self, struct dw2_hc *host, usb_speed_t speed,
		struct standardhub *hub, unsigned pindex);
void _usb_device(usb_dev_t *self);

boolean usb_dev_init(usb_dev_t *self);      // onto address state(phase 1)
boolean usb_dev_config(usb_dev_t *self);    // onto configured state(phase 2)

uint8_t usb_dev_get_addr(usb_dev_t *self);
usb_speed_t usb_dev_get_speed(usb_dev_t *self);

boolean usb_dev_is_split(usb_dev_t *self);
uint8_t usb_dev_get_hubaddr(usb_dev_t *self);
uint8_t usb_dev_get_hubport(usb_dev_t *self);

struct usb_ep *usb_dev_get_endpoint0(usb_dev_t *self);
struct dw2_hc *usb_dev_get_host(usb_dev_t *self);

const dev_desc_t *usb_dev_get_dev_desc(usb_dev_t *self);
const cfg_desc_t *usb_dev_get_cfg_descrr(usb_dev_t *self); // default config

// get next sub descriptor of type from configuration descriptor
const usb_desc_t *usb_dev_get_desc(usb_dev_t *self, uint8_t type);	// returns 0 if not found
void usb_dev_cfg_error(usb_dev_t *self, const char *source);
/// @brief アドレスをセット
void usb_dev_set_addr(usb_dev_t *self, uint8_t addr);          // xHCI: set slot ID

/// @brief デバイス再スキャン
boolean usb_dev_rescan_dev(usb_dev_t *self);
/// @brief デバイス削除
boolean usb_dev_remove_dev(usb_dev_t *self);
/// @brief 名前を取得
char *usb_dev_get_name(usb_dev_t *self, selector_t selector);        // 文字列はcallerが削除
/// @brief 名前をすべて取得
char *usb_dev_get_names(usb_dev_t *self);                      // 文字列はcallerが削除

#endif
