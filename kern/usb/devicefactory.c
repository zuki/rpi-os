#include "usb/devicefactory.h"
#include "types.h"
#include "string.h"
#include "kmalloc.h"
#include "console.h"

// devices of usb classes
#include "usb/standardhub.h"
#include "usb/lan7800.h"
#include "usb/cdcethernet.h"

usb_func_t *usb_devfactory_get_device(usb_func_t *parent, char *name)
{
    usb_func_t *result = 0;
    // USBハブ
    if (strcmp(name, "int9-0-0") == 0
     || strcmp(name, "int9-0-2") == 0)
    {
        usb_stdhub_t *dev = (usb_stdhub_t *)kmalloc(sizeof(usb_stdhub_t));
        usb_standardhub(dev, parent);
        result = (usb_func_t *)dev;
    }
    // CLAN7800 Ethernetコントローラ
    else if (strcmp(name, "ven424-7800") == 0)
    {
        lan7800_t *dev = (lan7800_t *)kmalloc(sizeof(lan7800_t));
        lan7800(dev, parent);
        result = (usb_func_t *)dev;
    }
    // CDC Ethernetデバイス
    else if (strcmp(name, "int2-6-0") == 0)
    {
        cdcether_t *dev = (cdcether_t *)kmalloc(sizeof(cdcether_t));
        cdcether(dev, parent);
        result = (usb_func_t *)dev;
    }

    if (result != 0)
        info("Using device/interface %s", name);

    kmfree(name);

    return result;
}
