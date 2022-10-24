#ifndef INC_USB_DW2ROOTPORT_H
#define INC_USB_DW2ROOTPORT_H

#include "usb/device.h"
#include "types.h"

struct dw2_hc;

typedef struct dw2_rport {
    struct dw2_hc   *host;
    usb_dev_t       *dev;
} dw2_rport_t;

void dw2_rport(dw2_rport_t *self, struct dw2_hc *host);
void _dw2_rport(dw2_rport_t *self);
boolean dw2_rport_init(dw2_rport_t *self);
boolean dw2_rport_rescan_dev(dw2_rport_t *self);
boolean dw2_rport_remove_dev(dw2_rport_t *self);
void dw2_rport_handle_port_status_change(dw2_rport_t *self);
void dw2_rport_port_status_changed(dw2_rport_t *self);

#endif
