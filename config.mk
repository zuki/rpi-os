RASPI := 3

ARCH := aarch64
#CROSS := aarch64-linux-gnu-
CROSS := aarch64-unknown-linux-gnu-
CC := $(CROSS)gcc
LD := $(CROSS)ld
OBJDUMP := $(CROSS)objdump
OBJCOPY := $(CROSS)objcopy
STRIP := $(CROSS)strip

QEMU := qemu-system-aarch64

