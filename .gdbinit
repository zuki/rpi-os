set architecture aarch64
file obj/kernel8.elf
target remote localhost:1234
set print pretty on
set logging off
set height 0

#watch *0xffff000038bfc000
#break proc.c:377
#break dw2_hc_alloc_wblock
#break dw2_hc
#break dw2_hc_init
#break dw2hcd.c:221
#break dw2rootport.c:64
#break device.c:388
break standardhub.c:156
break proc.c:235

#break usb/standardhub.c:220

# Modify the following path to support pwndbg
#source /mnt/d/Workspace/github/pwndbg/gdbinit.py
