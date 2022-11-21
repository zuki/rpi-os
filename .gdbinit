set architecture aarch64
file obj/kernel8.elf
target remote localhost:1234
set print pretty on
set logging off
set height 0

#break proc.c:377
#break dw2_hc_alloc_wblock
#break dw2_hc
#break dw2_hc_init
break proc.c:225
#break usb/standardhub.c:120

# Modify the following path to support pwndbg
#source /mnt/d/Workspace/github/pwndbg/gdbinit.py
