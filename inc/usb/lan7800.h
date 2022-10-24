//
// lan7800.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2019  R. Stange <rsta2@o2online.de>
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
#ifndef INC_LAN7800_H
#define INC_LAN7800_H

#include "types.h"
#include "usb/function.h"
#include "usb/endpoint.h"
#include "usb/request.h"
#include "netdevice.h"

// サイズ
#define HS_USB_PKT_SIZE         512

#define MAX_RX_FIFO_SIZE        (12 * 1024)
#define MAX_TX_FIFO_SIZE        (12 * 1024)
#define DEFAULT_BURST_CAP_SIZE  MAX_TX_FIFO_SIZE
#define DEFAULT_BULK_IN_DELAY   0x800

#define RX_HEADER_SIZE          (4 + 4 + 2)
#define TX_HEADER_SIZE          (4 + 4)

#define MAX_RX_FRAME_SIZE       (2*6 + 2 + 1500 + 4)

// USBベンダーリクエスト
#define WRITE_REGISTER          0xA0
#define READ_REGISTER           0xA1

// デバイスレジスタ: chap 15参照
#define ID_REV                  0x000
    #define ID_REV_CHIP_ID_MASK     0xFFFF0000
        #define ID_REV_CHIP_ID_7800     0x7800
#define INT_STS                 0x00C
    #define INT_STS_CLEAR_ALL       0xFFFFFFFF
#define HW_CFG                  0x010
    #define HW_CFG_CLK125_EN        0x02000000
    #define HW_CFG_REFCLK25_EN      0x01000000
    #define HW_CFG_LED3_EN          0x00800000
    #define HW_CFG_LED2_EN          0x00400000
    #define HW_CFG_LED1_EN          0x00200000
    #define HW_CFG_LED0_EN          0x00100000
    #define HW_CFG_EEE_PHY_LUSU     0x00020000
    #define HW_CFG_EEE_TSU          0x00010000
    #define HW_CFG_NETDET_STS       0x00008000
    #define HW_CFG_NETDET_EN        0x00004000
    #define HW_CFG_EEM              0x00002000
    #define HW_CFG_RST_PROTECT      0x00001000
    #define HW_CFG_CONNECT_BUF      0x00000400
    #define HW_CFG_CONNECT_EN       0x00000200
    #define HW_CFG_CONNECT_POL      0x00000100
    #define HW_CFG_SUSPEND_N_SEL_MASK    0x000000C0
        #define HW_CFG_SUSPEND_N_SEL_2      0x00000000
        #define HW_CFG_SUSPEND_N_SEL_12N    0x00000040
        #define HW_CFG_SUSPEND_N_SEL_012N   0x00000080
        #define HW_CFG_SUSPEND_N_SEL_0123N  0x000000C0
    #define HW_CFG_SUSPEND_N_POL    0x00000020
    #define HW_CFG_MEF              0x00000010
    #define HW_CFG_ETC              0x00000008
    #define HW_CFG_LRST             0x00000002
    #define HW_CFG_SRST             0x00000001
#define PMT_CTL                 0x014
    #define PMT_CTL_EEE_WAKEUP_EN   0x00002000
    #define PMT_CTL_EEE_WUPS        0x00001000
    #define PMT_CTL_MAC_SRST        0x00000800
    #define PMT_CTL_PHY_PWRUP       0x00000400
    #define PMT_CTL_RES_CLR_WKP_MASK 0x00000300
    #define PMT_CTL_RES_CLR_WKP_STS 0x00000200
    #define PMT_CTL_RES_CLR_WKP_EN  0x00000100
    #define PMT_CTL_READY           0x00000080
    #define PMT_CTL_SUS_MODE_MASK   0x00000060
    #define PMT_CTL_SUS_MODE_0      0x00000000
    #define PMT_CTL_SUS_MODE_1      0x00000020
    #define PMT_CTL_SUS_MODE_2      0x00000040
    #define PMT_CTL_SUS_MODE_3      0x00000060
    #define PMT_CTL_PHY_RST         0x00000010
    #define PMT_CTL_WOL_EN          0x00000008
    #define PMT_CTL_PHY_WAKE_EN     0x00000004
    #define PMT_CTL_WUPS_MASK       0x00000003
    #define PMT_CTL_WUPS_MLT        0x00000003
    #define PMT_CTL_WUPS_MAC        0x00000002
    #define PMT_CTL_WUPS_PHY        0x00000001
#define USB_CFG0                0x080
    #define USB_CFG_BIR             0x00000040
    #define USB_CFG_BCE             0x00000020
#define BURST_CAP               0x090
    #define BURST_CAP_SIZE_MASK_    0x000000FF
#define BULK_IN_DLY             0x094
    #define BULK_IN_DLY_MASK_       0x0000FFFF
#define INT_EP_CTL              0x098
    #define INT_EP_INTEP_ON         0x80000000
    #define INT_STS_EEE_TX_LPI_STRT_EN 0x04000000
    #define INT_STS_EEE_TX_LPI_STOP_EN 0x02000000
    #define INT_STS_EEE_RX_LPI_EN   0x01000000
    #define INT_EP_RDFO_EN          0x00400000
    #define INT_EP_TXE_EN           0x00200000
    #define INT_EP_TX_DIS_EN        0x00080000
    #define INT_EP_RX_DIS_EN        0x00040000
    #define INT_EP_PHY_INT_EN       0x00020000
    #define INT_EP_DP_INT_EN        0x00010000
    #define INT_EP_MAC_ERR_EN       0x00008000
    #define INT_EP_TDFU_EN          0x00004000
    #define INT_EP_TDFO_EN          0x00002000
    #define INT_EP_UTX_FP_EN        0x00001000
    #define INT_EP_GPIO_EN_MASK     0x00000FFF
#define RFE_CTL                 0x0B0
    #define RFE_CTL_IGMP_COE        0x00004000
    #define RFE_CTL_ICMP_COE        0x00002000
    #define RFE_CTL_TCPUDP_COE      0x00001000
    #define RFE_CTL_IP_COE          0x00000800
    #define RFE_CTL_BCAST_EN        0x00000400
    #define RFE_CTL_MCAST_EN        0x00000200
    #define RFE_CTL_UCAST_EN        0x00000100
    #define RFE_CTL_VLAN_STRIP      0x00000080
    #define RFE_CTL_DISCARD_UNTAGGED 0x00000040
    #define RFE_CTL_VLAN_FILTER     0x00000020
    #define RFE_CTL_SA_FILTER       0x00000010
    #define RFE_CTL_MCAST_HASH      0x00000008
    #define RFE_CTL_DA_HASH         0x00000004
    #define RFE_CTL_DA_PERFECT      0x00000002
    #define RFE_CTL_RST             0x00000001
#define FCT_RX_CTL              0x0C0
    #define FCT_RX_CTL_EN           0x80000000
    #define FCT_RX_CTL_RST          0x40000000
    #define FCT_RX_CTL_SBF          0x02000000
    #define FCT_RX_CTL_OVFL         0x01000000
    #define FCT_RX_CTL_DROP         0x00800000
    #define FCT_RX_CTL_NOT_EMPTY    0x00400000
    #define FCT_RX_CTL_EMPTY        0x00200000
    #define FCT_RX_CTL_DIS          0x00100000
    #define FCT_RX_CTL_USED_MASK    0x0000FFFF
#define FCT_TX_CTL              0x0C4
    #define FCT_TX_CTL_EN           0x80000000
    #define FCT_TX_CTL_RST          0x40000000
    #define FCT_TX_CTL_NOT_EMPTY    0x00400000
    #define FCT_TX_CTL_EMPTY        0x00200000
    #define FCT_TX_CTL_DIS          0x00100000
    #define FCT_TX_CTL_USED_MASK    0x0000FFFF
#define FCT_RX_FIFO_END         0x0C8
    #define FCT_RX_FIFO_END_MASK    0x0000007F
#define FCT_TX_FIFO_END         0x0CC
    #define FCT_TX_FIFO_END_MASK    0x0000003F
#define FCT_FLOW                0x0D0
#define MAC_CR                  0x100
    #define MAC_CR_GMII_EN          0x00080000
    #define MAC_CR_EEE_TX_CLK_STOP_EN 0x00040000
    #define MAC_CR_EEE_EN           0x00020000
    #define MAC_CR_EEE_TLAR_EN      0x00010000
    #define MAC_CR_ADP              0x00002000
    #define MAC_CR_AUTO_DUPLEX      0x00001000
    #define MAC_CR_AUTO_SPEED       0x00000800
    #define MAC_CR_LOOPBACK         0x00000400
    #define MAC_CR_BOLMT_MASK       0x000000C0
    #define MAC_CR_FULL_DUPLEX      0x00000008
    #define MAC_CR_SPEED_MASK       0x00000006
    #define MAC_CR_SPEED_1000       0x00000004
    #define MAC_CR_SPEED_100        0x00000002
    #define MAC_CR_SPEED_10         0x00000000
    #define MAC_CR_RST              0x00000001
#define MAC_RX                  0x104
    #define MAC_RX_MAX_SIZE_SHIFT   16
    #define MAC_RX_MAX_SIZE_MASK    0x3FFF0000
    #define MAC_RX_FCS_STRIP        0x00000010
    #define MAC_RX_VLAN_FSE         0x00000004
    #define MAC_RX_RXD              0x00000002
    #define MAC_RX_RXEN             0x00000001
#define MAC_TX                  0x108
    #define MAC_TX_BAD_FCS          0x00000004
    #define MAC_TX_TXD              0x00000002
    #define MAC_TX_TXEN             0x00000001
#define FLOW                    0x10C
#define RX_ADDRH                0x118
    #define RX_ADDRH_MASK_          0x0000FFFF
#define RX_ADDRL                0x11C
    #define RX_ADDRL_MASK_          0xFFFFFFFF
#define MII_ACC                 0x120
    #define MII_ACC_PHY_ADDR_SHIFT  11
    #define MII_ACC_PHY_ADDR_MASK   0x0000F800
        #define PHY_ADDRESS             1
    #define MII_ACC_MIIRINDA_SHIFT  6
    #define MII_ACC_MIIRINDA_MASK   0x000007C0
    #define MII_ACC_MII_READ        0x00000000
    #define MII_ACC_MII_WRITE       0x00000002
    #define MII_ACC_MII_BUSY        0x00000001
#define MII_DATA                0x124
    #define MII_DATA_MASK           0x0000FFFF
#define MAF_BASE                0x400
    #define MAF_HIX                 0x00
    #define MAF_LOX                 0x04
    #define NUM_OF_MAF              33
    #define MAF_HI_BEGIN            (MAF_BASE + MAF_HIX)
    #define MAF_LO_BEGIN            (MAF_BASE + MAF_LOX)
    #define MAF_HI(index)           (MAF_BASE + (8 * (index)) + (MAF_HIX))
    #define MAF_LO(index)           (MAF_BASE + (8 * (index)) + (MAF_LOX))
    #define MAF_HI_VALID            0x80000000
    #define MAF_HI_TYPE_MASK        0x40000000
    #define MAF_HI_TYPE_SRC         0x40000000
    #define MAF_HI_TYPE_DST         0x00000000
    #define MAF_HI_ADDR_MASK        0x0000FFFF
    #define MAF_LO_ADDR_MASK        0xFFFFFFFF

// TX コマンド A                                FCS: Frame Check Sequence
#define TX_CMD_A_FCS            0x00400000      // 22: Insert FCS and Pad
#define TX_CMD_A_LEN_MASK       0x000FFFFF      // 19-0: Frame length

// RX コマンド A
#define RX_CMD_A_RED            0x00400000      // 22: Receive Error Detected
#define RX_CMD_A_LEN_MASK       0x00003FFF      // 13-0: Frame length

typedef struct lan7800 {
    usb_func_t   usb_func;
    net_dev_t   *net_dev;
    usb_ep_t    *bulk_in;
    usb_ep_t    *bulk_out;
    char         macaddr[MAC_ADDRESS_SIZE];
    uint8_t     *tx_buffer;
} lan7800_t;

void lan7800(lan7800_t *self, usb_func_t *func);
void _lan7800(lan7800_t *self);

boolean lan7800_send_frame(lan7800_t *self, const void *buffer, uint32_t len);
boolean lan7800_receive_frame(lan7800_t *self, void *buff, uint32_t *resultlen);
int lan7800_is_linkup(lan7800_t *self);

const char *lan7800_get_macaddr(lan7800_t *self);
net_speed_t lan7800_get_linkspeed(lan7800_t *self);

#endif
