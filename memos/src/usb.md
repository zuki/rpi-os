# USBを追加

```
[1]HCDInitialise: VenderID=0x4f54294a, UserID=0x0
[1]HCDInitialise: DWC_CORE_HARDWARE=0x10000044_250dc016_00000000

[1]HCDInitialise: HCD: Hardware: OT2.94a (BCM00000).
[1]HCDInitialise: HCD: High speed physical unsupported:
[1]UsbInitialise: FATAL ERROR: HCD failed to initialise.

 +-Unconfigured Device id: 3 port: 0 speed: High packetsize: 16

 (qemu) info qtree
bus: main-system-bus
  type System
  dev: bcm2836-control, id ""
    gpio-out "fiq" 4
    gpio-out "irq" 4
    gpio-in "gpu-fiq" 1
    gpio-in "gpu-irq" 1
    gpio-in "cntvirq" 4
    gpio-in "cnthpirq" 4
    gpio-in "cntpnsirq" 4
    gpio-in "cntpsirq" 4
    mmio 0000000040000000/0000000000000100
(...)
  dev: dwc2-usb, id ""
    gpio-out "sysbus-irq" 1
    usb_version = 2 (0x2)
    mmio ffffffffffffffff/0000000000011000
    bus: usb-bus.0
      type usb-bus
      dev: usb-hub, id ""
        ports = 8 (0x8)
        port-power = false
        port = ""
        serial = ""
        msos-desc = true
        pcap = ""
        addr 0.0, port 1, speed 12, name QEMU USB Hub, attached
      dev: usb-net, id ""
        mac = "52:54:00:12:34:57"
        netdev = "net1"
        port = ""
        serial = ""
        msos-desc = true
        pcap = ""
        addr 0.0, port 1.1, speed 12, name QEMU USB Network Interface, attached
  dev: bcm2835-mphi, id ""
    gpio-out "sysbus-irq" 1
    mmio ffffffffffffffff/0000000000001000

```

## 実機で実行

```
[0]HCDInitialise: VenderID=0x4f54280a, UserID=0x2708a000

[0]HCDInitialise: HCD: Hardware: OT2.80a (BCM2708a).
[0]EnumerateDevice: HCD: Attach Device USB Root Hub. Address:1 Class:9 USB:2.0,.
[0]EnumerateDevice: HCD:  -Product:       FAKED Root Hub (tm).
[0]EnumerateDevice: HCD:  -Configuration: FAKE config string.
[0]HcdProcessRootHubMessage: Physical host power on
[0]HCDChannelTransfer: Result: -16 Action: 0x20010100 tempInt: 0x0000000a tempS0
[0]HCDChannelTransfer: Result: -16 Action: 0x20020200 tempInt: 0x0000000a tempS0
[0]HCDChannelTransfer: Result: -10 Action: 0x40020300 tempInt: 0x0000000a tempS0
[0]HCDSumbitControlMessage: HCD: Could not transfer DATA to device 0.
[0]EnumerateDevice: Enumeration: Step 1 on device 2 failed, Result: %#x.

 +-USB Root Hub id: 1 port: 0 speed: Full packetsize: 64
    ?-New Device (Not Ready) id: 0 port: 0 speed: High packetsize: 8
```

## QEMUのraspiのUSB対応はusb.cが必要とする機能を提供していないらしい

- qemuのhcd-dwc2 USBコントローラはfull speedが前提らしい
  - [arm raspi2/raspi3 emulation has no USB support](https://bugs.launchpad.net/qemu/+bug/1772165)における作者の回答より

    > But note that if you absolutely must pass-through a USB device from the host, it probably won't work.
    > That's because the dwc2 controller emulation is connected through a full-speed hub emulation, so unless
    > your USB device is connected at full-speed on the host, it probably won't work.

    ```
    // QEMU/hw/usb/hcd-dwc2.c
    static void dwc2_reset_enter(Object *obj, ResetType type)
    {
    (...)
        s->guid = 0;
        s->gsnpsid = 0x4f54294a;
        s->ghwcfg1 = 0;
        s->ghwcfg2 = (8 << GHWCFG2_DEV_TOKEN_Q_DEPTH_SHIFT) |                   // ただし、GHWCFG2_HS_PHY_TYPEだけでなく
                    (4 << GHWCFG2_HOST_PERIO_TX_Q_DEPTH_SHIFT) |                // GHWCFG2_FS_PHY_TYPEも指定していない
                    (4 << GHWCFG2_NONPERIO_TX_Q_DEPTH_SHIFT) |
                    GHWCFG2_DYNAMIC_FIFO |
                    GHWCFG2_PERIO_EP_SUPPORTED |
                    ((DWC2_NB_CHAN - 1) << GHWCFG2_NUM_HOST_CHAN_SHIFT) |
                    (GHWCFG2_INT_DMA_ARCH << GHWCFG2_ARCHITECTURE_SHIFT) |
                    (GHWCFG2_OP_MODE_NO_SRP_CAPABLE_HOST << GHWCFG2_OP_MODE_SHIFT);
        s->ghwcfg3 = (4096 << GHWCFG3_DFIFO_DEPTH_SHIFT) |
                    (4 << GHWCFG3_PACKET_SIZE_CNTR_WIDTH_SHIFT) |
                    (4 << GHWCFG3_XFER_SIZE_CNTR_WIDTH_SHIFT);
        s->ghwcfg4 = 0;
    ```

- 一方、usb.cはhigh speedを前提に書かれている

    ```
    // usb.c#HCDInitialise()
    if (DWC_CORE_HARDWARE->HighSpeedPhysical == NotSupported) {     // high speed転送が必要
        LOG("HCD: High speed physical unsupported: ");
        return ErrorIncompatible;                                   // Return hardware incompatible
    }
    ```

## 一旦、あきらめる
