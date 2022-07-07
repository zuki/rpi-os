# 05: muslをインストール

- Macには`/lib`がない（作成できない）
- `$HOME/musl`にインストール

```
$ mkdir -p $HOME/musl
$ cd $XV6/libc
$ export CROSS_COMPILE=aarch64-unknown-linux-gnu- && ./configure --target=aarch64 --prefix=$HOME/musl
$ make
$ make
sh tools/musl-gcc.specs.sh "$HOME/musl/include" "$HOME/musl/lib" "/lib/ld-musl-aarch64.so.1" > lib/musl-gcc.specs
printf '#!/bin/sh\nexec "${REALGCC:-aarch64-unknown-linux-gnu-gcc}" "$@" -specs "%s/musl-gcc.specs"\n' "$HOME/musl/lib" > obj/musl-gcc
chmod +x obj/musl-gcc
$ make install
./tools/install.sh -D -m 644 lib/Scrt1.o $HOME/musl/lib/Scrt1.o
./tools/install.sh -D -m 644 lib/crti.o $HOME/musl/lib/crti.o
./tools/install.sh -D -m 644 lib/crtn.o $HOME/musl/lib/crtn.o
./tools/install.sh -D -m 644 lib/crt1.o $HOME/musl/lib/crt1.o
./tools/install.sh -D -m 644 lib/rcrt1.o $HOME/musl/lib/rcrt1.o
./tools/install.sh -D -m 644 lib/libc.a $HOME/musl/lib/libc.a
./tools/install.sh -D -m 755 lib/libc.so $HOME/musl/lib/libc.so
./tools/install.sh -D -m 644 lib/libm.a $HOME/musl/lib/libm.a
./tools/install.sh -D -m 644 lib/librt.a $HOME/musl/lib/librt.a
./tools/install.sh -D -m 644 lib/libpthread.a $HOME/musl/lib/libpthread.a
./tools/install.sh -D -m 644 lib/libcrypt.a $HOME/musl/lib/libcrypt.a
./tools/install.sh -D -m 644 lib/libutil.a $HOME/musl/lib/libutil.a
./tools/install.sh -D -m 644 lib/libxnet.a $HOME/musl/lib/libxnet.a
./tools/install.sh -D -m 644 lib/libresolv.a $HOME/musl/lib/libresolv.a
./tools/install.sh -D -m 644 lib/libdl.a $HOME/musl/lib/libdl.a
./tools/install.sh -D -m 644 lib/musl-gcc.specs $HOME/musl/lib/musl-gcc.specs
./tools/install.sh -D -l $HOME/musl/lib/libc.so /lib/ld-musl-aarch64.so.1 || true
mkdir: /lib: Read-only file system
./tools/install.sh -D -m 644 include/aio.h $HOME/musl/include/aio.h
./tools/install.sh -D -m 644 include/alloca.h $HOME/musl/include/alloca.h
./tools/install.sh -D -m 644 include/ar.h $HOME/musl/include/ar.h
./tools/install.sh -D -m 644 include/arpa/ftp.h $HOME/musl/include/arpa/ftp.h
./tools/install.sh -D -m 644 include/arpa/inet.h $HOME/musl/include/arpa/inet.h
./tools/install.sh -D -m 644 include/arpa/nameser.h $HOME/musl/include/arpa/nameser.h
./tools/install.sh -D -m 644 include/arpa/nameser_compat.h $HOME/musl/include/arpa/nameser_compat.h
./tools/install.sh -D -m 644 include/arpa/telnet.h $HOME/musl/include/arpa/telnet.h
./tools/install.sh -D -m 644 include/arpa/tftp.h $HOME/musl/include/arpa/tftp.h
./tools/install.sh -D -m 644 include/assert.h $HOME/musl/include/assert.h
./tools/install.sh -D -m 644 obj/include/bits/alltypes.h $HOME/musl/include/bits/alltypes.h
./tools/install.sh -D -m 644 arch/generic/bits/dirent.h $HOME/musl/include/bits/dirent.h
./tools/install.sh -D -m 644 arch/generic/bits/errno.h $HOME/musl/include/bits/errno.h
./tools/install.sh -D -m 644 arch/aarch64/bits/fcntl.h $HOME/musl/include/bits/fcntl.h
./tools/install.sh -D -m 644 arch/aarch64/bits/fenv.h $HOME/musl/include/bits/fenv.h
./tools/install.sh -D -m 644 arch/aarch64/bits/float.h $HOME/musl/include/bits/float.h
./tools/install.sh -D -m 644 arch/aarch64/bits/hwcap.h $HOME/musl/include/bits/hwcap.h
./tools/install.sh -D -m 644 arch/generic/bits/io.h $HOME/musl/include/bits/io.h
./tools/install.sh -D -m 644 arch/generic/bits/ioctl.h $HOME/musl/include/bits/ioctl.h
./tools/install.sh -D -m 644 arch/generic/bits/ioctl_fix.h $HOME/musl/include/bits/ioctl_fix.h
./tools/install.sh -D -m 644 arch/generic/bits/ipc.h $HOME/musl/include/bits/ipc.h
./tools/install.sh -D -m 644 arch/generic/bits/ipcstat.h $HOME/musl/include/bits/ipcstat.h
./tools/install.sh -D -m 644 arch/generic/bits/kd.h $HOME/musl/include/bits/kd.h
./tools/install.sh -D -m 644 arch/generic/bits/limits.h $HOME/musl/include/bits/limits.h
./tools/install.sh -D -m 644 arch/generic/bits/link.h $HOME/musl/include/bits/link.h
./tools/install.sh -D -m 644 arch/generic/bits/mman.h $HOME/musl/include/bits/mman.h
./tools/install.sh -D -m 644 arch/generic/bits/msg.h $HOME/musl/include/bits/msg.h
./tools/install.sh -D -m 644 arch/generic/bits/poll.h $HOME/musl/include/bits/poll.h
./tools/install.sh -D -m 644 arch/aarch64/bits/posix.h $HOME/musl/include/bits/posix.h
./tools/install.sh -D -m 644 arch/generic/bits/ptrace.h $HOME/musl/include/bits/ptrace.h
./tools/install.sh -D -m 644 arch/aarch64/bits/reg.h $HOME/musl/include/bits/reg.h
./tools/install.sh -D -m 644 arch/generic/bits/resource.h $HOME/musl/include/bits/resource.h
./tools/install.sh -D -m 644 arch/generic/bits/sem.h $HOME/musl/include/bits/sem.h
./tools/install.sh -D -m 644 arch/aarch64/bits/setjmp.h $HOME/musl/include/bits/setjmp.h
./tools/install.sh -D -m 644 arch/generic/bits/shm.h $HOME/musl/include/bits/shm.h
./tools/install.sh -D -m 644 arch/aarch64/bits/signal.h $HOME/musl/include/bits/signal.h
./tools/install.sh -D -m 644 arch/generic/bits/socket.h $HOME/musl/include/bits/socket.h
./tools/install.sh -D -m 644 arch/generic/bits/soundcard.h $HOME/musl/include/bits/soundcard.h
./tools/install.sh -D -m 644 arch/aarch64/bits/stat.h $HOME/musl/include/bits/stat.h
./tools/install.sh -D -m 644 arch/generic/bits/statfs.h $HOME/musl/include/bits/statfs.h
./tools/install.sh -D -m 644 arch/aarch64/bits/stdint.h $HOME/musl/include/bits/stdint.h
./tools/install.sh -D -m 644 obj/include/bits/syscall.h $HOME/musl/include/bits/syscall.h
./tools/install.sh -D -m 644 arch/generic/bits/termios.h $HOME/musl/include/bits/termios.h
./tools/install.sh -D -m 644 arch/aarch64/bits/user.h $HOME/musl/include/bits/user.h
./tools/install.sh -D -m 644 arch/generic/bits/vt.h $HOME/musl/include/bits/vt.h
./tools/install.sh -D -m 644 include/byteswap.h $HOME/musl/include/byteswap.h
./tools/install.sh -D -m 644 include/complex.h $HOME/musl/include/complex.h
./tools/install.sh -D -m 644 include/cpio.h $HOME/musl/include/cpio.h
./tools/install.sh -D -m 644 include/crypt.h $HOME/musl/include/crypt.h
./tools/install.sh -D -m 644 include/ctype.h $HOME/musl/include/ctype.h
./tools/install.sh -D -m 644 include/dirent.h $HOME/musl/include/dirent.h
./tools/install.sh -D -m 644 include/dlfcn.h $HOME/musl/include/dlfcn.h
./tools/install.sh -D -m 644 include/elf.h $HOME/musl/include/elf.h
./tools/install.sh -D -m 644 include/endian.h $HOME/musl/include/endian.h
./tools/install.sh -D -m 644 include/err.h $HOME/musl/include/err.h
./tools/install.sh -D -m 644 include/errno.h $HOME/musl/include/errno.h
./tools/install.sh -D -m 644 include/fcntl.h $HOME/musl/include/fcntl.h
./tools/install.sh -D -m 644 include/features.h $HOME/musl/include/features.h
./tools/install.sh -D -m 644 include/fenv.h $HOME/musl/include/fenv.h
./tools/install.sh -D -m 644 include/float.h $HOME/musl/include/float.h
./tools/install.sh -D -m 644 include/fmtmsg.h $HOME/musl/include/fmtmsg.h
./tools/install.sh -D -m 644 include/fnmatch.h $HOME/musl/include/fnmatch.h
./tools/install.sh -D -m 644 include/ftw.h $HOME/musl/include/ftw.h
./tools/install.sh -D -m 644 include/getopt.h $HOME/musl/include/getopt.h
./tools/install.sh -D -m 644 include/glob.h $HOME/musl/include/glob.h
./tools/install.sh -D -m 644 include/grp.h $HOME/musl/include/grp.h
./tools/install.sh -D -m 644 include/iconv.h $HOME/musl/include/iconv.h
./tools/install.sh -D -m 644 include/ifaddrs.h $HOME/musl/include/ifaddrs.h
./tools/install.sh -D -m 644 include/inttypes.h $HOME/musl/include/inttypes.h
./tools/install.sh -D -m 644 include/iso646.h $HOME/musl/include/iso646.h
./tools/install.sh -D -m 644 include/langinfo.h $HOME/musl/include/langinfo.h
./tools/install.sh -D -m 644 include/lastlog.h $HOME/musl/include/lastlog.h
./tools/install.sh -D -m 644 include/libgen.h $HOME/musl/include/libgen.h
./tools/install.sh -D -m 644 include/libintl.h $HOME/musl/include/libintl.h
./tools/install.sh -D -m 644 include/limits.h $HOME/musl/include/limits.h
./tools/install.sh -D -m 644 include/link.h $HOME/musl/include/link.h
./tools/install.sh -D -m 644 include/locale.h $HOME/musl/include/locale.h
./tools/install.sh -D -m 644 include/malloc.h $HOME/musl/include/malloc.h
./tools/install.sh -D -m 644 include/math.h $HOME/musl/include/math.h
./tools/install.sh -D -m 644 include/memory.h $HOME/musl/include/memory.h
./tools/install.sh -D -m 644 include/mntent.h $HOME/musl/include/mntent.h
./tools/install.sh -D -m 644 include/monetary.h $HOME/musl/include/monetary.h
./tools/install.sh -D -m 644 include/mqueue.h $HOME/musl/include/mqueue.h
./tools/install.sh -D -m 644 include/net/ethernet.h $HOME/musl/include/net/ethernet.h
./tools/install.sh -D -m 644 include/net/if.h $HOME/musl/include/net/if.h
./tools/install.sh -D -m 644 include/net/if_arp.h $HOME/musl/include/net/if_arp.h
./tools/install.sh -D -m 644 include/net/route.h $HOME/musl/include/net/route.h
./tools/install.sh -D -m 644 include/netdb.h $HOME/musl/include/netdb.h
./tools/install.sh -D -m 644 include/netinet/ether.h $HOME/musl/include/netinet/ether.h
./tools/install.sh -D -m 644 include/netinet/icmp6.h $HOME/musl/include/netinet/icmp6.h
./tools/install.sh -D -m 644 include/netinet/if_ether.h $HOME/musl/include/netinet/if_ether.h
./tools/install.sh -D -m 644 include/netinet/igmp.h $HOME/musl/include/netinet/igmp.h
./tools/install.sh -D -m 644 include/netinet/in.h $HOME/musl/include/netinet/in.h
./tools/install.sh -D -m 644 include/netinet/in_systm.h $HOME/musl/include/netinet/in_systm.h
./tools/install.sh -D -m 644 include/netinet/ip.h $HOME/musl/include/netinet/ip.h
./tools/install.sh -D -m 644 include/netinet/ip6.h $HOME/musl/include/netinet/ip6.h
./tools/install.sh -D -m 644 include/netinet/ip_icmp.h $HOME/musl/include/netinet/ip_icmp.h
./tools/install.sh -D -m 644 include/netinet/tcp.h $HOME/musl/include/netinet/tcp.h
./tools/install.sh -D -m 644 include/netinet/udp.h $HOME/musl/include/netinet/udp.h
./tools/install.sh -D -m 644 include/netpacket/packet.h $HOME/musl/include/netpacket/packet.h
./tools/install.sh -D -m 644 include/nl_types.h $HOME/musl/include/nl_types.h
./tools/install.sh -D -m 644 include/paths.h $HOME/musl/include/paths.h
./tools/install.sh -D -m 644 include/poll.h $HOME/musl/include/poll.h
./tools/install.sh -D -m 644 include/pthread.h $HOME/musl/include/pthread.h
./tools/install.sh -D -m 644 include/pty.h $HOME/musl/include/pty.h
./tools/install.sh -D -m 644 include/pwd.h $HOME/musl/include/pwd.h
./tools/install.sh -D -m 644 include/regex.h $HOME/musl/include/regex.h
./tools/install.sh -D -m 644 include/resolv.h $HOME/musl/include/resolv.h
./tools/install.sh -D -m 644 include/sched.h $HOME/musl/include/sched.h
./tools/install.sh -D -m 644 include/scsi/scsi.h $HOME/musl/include/scsi/scsi.h
./tools/install.sh -D -m 644 include/scsi/scsi_ioctl.h $HOME/musl/include/scsi/scsi_ioctl.h
./tools/install.sh -D -m 644 include/scsi/sg.h $HOME/musl/include/scsi/sg.h
./tools/install.sh -D -m 644 include/search.h $HOME/musl/include/search.h
./tools/install.sh -D -m 644 include/semaphore.h $HOME/musl/include/semaphore.h
./tools/install.sh -D -m 644 include/setjmp.h $HOME/musl/include/setjmp.h
./tools/install.sh -D -m 644 include/shadow.h $HOME/musl/include/shadow.h
./tools/install.sh -D -m 644 include/signal.h $HOME/musl/include/signal.h
./tools/install.sh -D -m 644 include/spawn.h $HOME/musl/include/spawn.h
./tools/install.sh -D -m 644 include/stdalign.h $HOME/musl/include/stdalign.h
./tools/install.sh -D -m 644 include/stdarg.h $HOME/musl/include/stdarg.h
./tools/install.sh -D -m 644 include/stdbool.h $HOME/musl/include/stdbool.h
./tools/install.sh -D -m 644 include/stdc-predef.h $HOME/musl/include/stdc-predef.h
./tools/install.sh -D -m 644 include/stddef.h $HOME/musl/include/stddef.h
./tools/install.sh -D -m 644 include/stdint.h $HOME/musl/include/stdint.h
./tools/install.sh -D -m 644 include/stdio.h $HOME/musl/include/stdio.h
./tools/install.sh -D -m 644 include/stdio_ext.h $HOME/musl/include/stdio_ext.h
./tools/install.sh -D -m 644 include/stdlib.h $HOME/musl/include/stdlib.h
./tools/install.sh -D -m 644 include/stdnoreturn.h $HOME/musl/include/stdnoreturn.h
./tools/install.sh -D -m 644 include/string.h $HOME/musl/include/string.h
./tools/install.sh -D -m 644 include/strings.h $HOME/musl/include/strings.h
./tools/install.sh -D -m 644 include/stropts.h $HOME/musl/include/stropts.h
./tools/install.sh -D -m 644 include/sys/acct.h $HOME/musl/include/sys/acct.h
./tools/install.sh -D -m 644 include/sys/auxv.h $HOME/musl/include/sys/auxv.h
./tools/install.sh -D -m 644 include/sys/cachectl.h $HOME/musl/include/sys/cachectl.h
./tools/install.sh -D -m 644 include/sys/dir.h $HOME/musl/include/sys/dir.h
./tools/install.sh -D -m 644 include/sys/epoll.h $HOME/musl/include/sys/epoll.h
./tools/install.sh -D -m 644 include/sys/errno.h $HOME/musl/include/sys/errno.h
./tools/install.sh -D -m 644 include/sys/eventfd.h $HOME/musl/include/sys/eventfd.h
./tools/install.sh -D -m 644 include/sys/fanotify.h $HOME/musl/include/sys/fanotify.h
./tools/install.sh -D -m 644 include/sys/fcntl.h $HOME/musl/include/sys/fcntl.h
./tools/install.sh -D -m 644 include/sys/file.h $HOME/musl/include/sys/file.h
./tools/install.sh -D -m 644 include/sys/fsuid.h $HOME/musl/include/sys/fsuid.h
./tools/install.sh -D -m 644 include/sys/inotify.h $HOME/musl/include/sys/inotify.h
./tools/install.sh -D -m 644 include/sys/io.h $HOME/musl/include/sys/io.h
./tools/install.sh -D -m 644 include/sys/ioctl.h $HOME/musl/include/sys/ioctl.h
./tools/install.sh -D -m 644 include/sys/ipc.h $HOME/musl/include/sys/ipc.h
./tools/install.sh -D -m 644 include/sys/kd.h $HOME/musl/include/sys/kd.h
./tools/install.sh -D -m 644 include/sys/klog.h $HOME/musl/include/sys/klog.h
./tools/install.sh -D -m 644 include/sys/membarrier.h $HOME/musl/include/sys/membarrier.h
./tools/install.sh -D -m 644 include/sys/mman.h $HOME/musl/include/sys/mman.h
./tools/install.sh -D -m 644 include/sys/mount.h $HOME/musl/include/sys/mount.h
./tools/install.sh -D -m 644 include/sys/msg.h $HOME/musl/include/sys/msg.h
./tools/install.sh -D -m 644 include/sys/mtio.h $HOME/musl/include/sys/mtio.h
./tools/install.sh -D -m 644 include/sys/param.h $HOME/musl/include/sys/param.h
./tools/install.sh -D -m 644 include/sys/personality.h $HOME/musl/include/sys/personality.h
./tools/install.sh -D -m 644 include/sys/poll.h $HOME/musl/include/sys/poll.h
./tools/install.sh -D -m 644 include/sys/prctl.h $HOME/musl/include/sys/prctl.h
./tools/install.sh -D -m 644 include/sys/procfs.h $HOME/musl/include/sys/procfs.h
./tools/install.sh -D -m 644 include/sys/ptrace.h $HOME/musl/include/sys/ptrace.h
./tools/install.sh -D -m 644 include/sys/quota.h $HOME/musl/include/sys/quota.h
./tools/install.sh -D -m 644 include/sys/random.h $HOME/musl/include/sys/random.h
./tools/install.sh -D -m 644 include/sys/reboot.h $HOME/musl/include/sys/reboot.h
./tools/install.sh -D -m 644 include/sys/reg.h $HOME/musl/include/sys/reg.h
./tools/install.sh -D -m 644 include/sys/resource.h $HOME/musl/include/sys/resource.h
./tools/install.sh -D -m 644 include/sys/select.h $HOME/musl/include/sys/select.h
./tools/install.sh -D -m 644 include/sys/sem.h $HOME/musl/include/sys/sem.h
./tools/install.sh -D -m 644 include/sys/sendfile.h $HOME/musl/include/sys/sendfile.h
./tools/install.sh -D -m 644 include/sys/shm.h $HOME/musl/include/sys/shm.h
./tools/install.sh -D -m 644 include/sys/signal.h $HOME/musl/include/sys/signal.h
./tools/install.sh -D -m 644 include/sys/signalfd.h $HOME/musl/include/sys/signalfd.h
./tools/install.sh -D -m 644 include/sys/socket.h $HOME/musl/include/sys/socket.h
./tools/install.sh -D -m 644 include/sys/soundcard.h $HOME/musl/include/sys/soundcard.h
./tools/install.sh -D -m 644 include/sys/stat.h $HOME/musl/include/sys/stat.h
./tools/install.sh -D -m 644 include/sys/statfs.h $HOME/musl/include/sys/statfs.h
./tools/install.sh -D -m 644 include/sys/statvfs.h $HOME/musl/include/sys/statvfs.h
./tools/install.sh -D -m 644 include/sys/stropts.h $HOME/musl/include/sys/stropts.h
./tools/install.sh -D -m 644 include/sys/swap.h $HOME/musl/include/sys/swap.h
./tools/install.sh -D -m 644 include/sys/syscall.h $HOME/musl/include/sys/syscall.h
./tools/install.sh -D -m 644 include/sys/sysinfo.h $HOME/musl/include/sys/sysinfo.h
./tools/install.sh -D -m 644 include/sys/syslog.h $HOME/musl/include/sys/syslog.h
./tools/install.sh -D -m 644 include/sys/sysmacros.h $HOME/musl/include/sys/sysmacros.h
./tools/install.sh -D -m 644 include/sys/termios.h $HOME/musl/include/sys/termios.h
./tools/install.sh -D -m 644 include/sys/time.h $HOME/musl/include/sys/time.h
./tools/install.sh -D -m 644 include/sys/timeb.h $HOME/musl/include/sys/timeb.h
./tools/install.sh -D -m 644 include/sys/timerfd.h $HOME/musl/include/sys/timerfd.h
./tools/install.sh -D -m 644 include/sys/times.h $HOME/musl/include/sys/times.h
./tools/install.sh -D -m 644 include/sys/timex.h $HOME/musl/include/sys/timex.h
./tools/install.sh -D -m 644 include/sys/ttydefaults.h $HOME/musl/include/sys/ttydefaults.h
./tools/install.sh -D -m 644 include/sys/types.h $HOME/musl/include/sys/types.h
./tools/install.sh -D -m 644 include/sys/ucontext.h $HOME/musl/include/sys/ucontext.h
./tools/install.sh -D -m 644 include/sys/uio.h $HOME/musl/include/sys/uio.h
./tools/install.sh -D -m 644 include/sys/un.h $HOME/musl/include/sys/un.h
./tools/install.sh -D -m 644 include/sys/user.h $HOME/musl/include/sys/user.h
./tools/install.sh -D -m 644 include/sys/utsname.h $HOME/musl/include/sys/utsname.h
./tools/install.sh -D -m 644 include/sys/vfs.h $HOME/musl/include/sys/vfs.h
./tools/install.sh -D -m 644 include/sys/vt.h $HOME/musl/include/sys/vt.h
./tools/install.sh -D -m 644 include/sys/wait.h $HOME/musl/include/sys/wait.h
./tools/install.sh -D -m 644 include/sys/xattr.h $HOME/musl/include/sys/xattr.h
./tools/install.sh -D -m 644 include/syscall.h $HOME/musl/include/syscall.h
./tools/install.sh -D -m 644 include/sysexits.h $HOME/musl/include/sysexits.h
./tools/install.sh -D -m 644 include/syslog.h $HOME/musl/include/syslog.h
./tools/install.sh -D -m 644 include/tar.h $HOME/musl/include/tar.h
./tools/install.sh -D -m 644 include/termios.h $HOME/musl/include/termios.h
./tools/install.sh -D -m 644 include/tgmath.h $HOME/musl/include/tgmath.h
./tools/install.sh -D -m 644 include/threads.h $HOME/musl/include/threads.h
./tools/install.sh -D -m 644 include/time.h $HOME/musl/include/time.h
./tools/install.sh -D -m 644 include/uchar.h $HOME/musl/include/uchar.h
./tools/install.sh -D -m 644 include/ucontext.h $HOME/musl/include/ucontext.h
./tools/install.sh -D -m 644 include/ulimit.h $HOME/musl/include/ulimit.h
./tools/install.sh -D -m 644 include/unistd.h $HOME/musl/include/unistd.h
./tools/install.sh -D -m 644 include/utime.h $HOME/musl/include/utime.h
./tools/install.sh -D -m 644 include/utmp.h $HOME/musl/include/utmp.h
./tools/install.sh -D -m 644 include/utmpx.h $HOME/musl/include/utmpx.h
./tools/install.sh -D -m 644 include/values.h $HOME/musl/include/values.h
./tools/install.sh -D -m 644 include/wait.h $HOME/musl/include/wait.h
./tools/install.sh -D -m 644 include/wchar.h $HOME/musl/include/wchar.h
./tools/install.sh -D -m 644 include/wctype.h $HOME/musl/include/wctype.h
./tools/install.sh -D -m 644 include/wordexp.h $HOME/musl/include/wordexp.h
./tools/install.sh -D obj/musl-gcc $HOME/musl/bin/musl-gcc
```

## `/lib/ld-musl-aarch64.so.1`対応

- `$HOME/musl/lib/musl-gcc.specs`を編集
- `usr/`配下のCCに`$HOME/musl/bin/musl-gcc`を使用する

```
$ vi $HOME/musl/lib/musl-gcc.specs
$ diff -uw musl-gcc.specs.org musl-gcc.specs
--- musl-gcc.specs.org	2022-06-14 09:44:39.000000000 +0900
+++ musl-gcc.specs	2022-06-14 09:47:51.000000000 +0900
@@ -19,7 +19,7 @@
 crtendS.o%s $HOME/musl/lib/crtn.o

 *link:
--dynamic-linker /lib/ld-musl-aarch64.so.1 -nostdlib %{shared:-shared} %{static:-static} %{rdynamic:-export-dynamic}
+-dynamic-linker $HOME/musl/lib/ld-musl-aarch64.so.1 -nostdlib %{shared:-shared} %{static:-static} %{rdynamic:-export-dynamic}

 *esp_link:

$ echo "export $HOME/musl/bin" >> $HOME/.bash_profile
$ musl-gcc -o hello-musl hello.c
$ file hello-musl
hello-musl: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter $HOME/musl/lib/ld-musl-aarch64.so.1, not stripped
$ aarch64-unknown-linux-gnu-readelf -l hello-musl

Elf file type is EXEC (Executable file)
Entry point 0x400580
There are 7 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000400040 0x0000000000400040
                 0x0000000000000188 0x0000000000000188  R E    0x8
  INTERP         0x00000000000001c8 0x00000000004001c8 0x00000000004001c8
                 0x000000000000002c 0x000000000000002c  R      0x1
      [Requesting program interpreter: $HOME/musl/lib/ld-musl-aarch64.so.1]
  LOAD           0x0000000000000000 0x0000000000400000 0x0000000000400000
                 0x000000000000075c 0x000000000000075c  R E    0x10000
  LOAD           0x0000000000000e00 0x0000000000410e00 0x0000000000410e00
                 0x0000000000000220 0x0000000000000228  RW     0x10000
  DYNAMIC        0x0000000000000e10 0x0000000000410e10 0x0000000000410e10
                 0x00000000000001a0 0x00000000000001a0  RW     0x8
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10
  GNU_RELRO      0x0000000000000e00 0x0000000000410e00 0x0000000000410e00
                 0x0000000000000200 0x0000000000000200  R      0x1
```

## `Makefile`, `usr/Makefile`を編集

- インストールしたmuslを使うよう編集
- `make clean`で`libc`と`boot`をcleanしないように変更

    ```
    $ vi Makefile
    diff --git a/Makefile b/Makefile
    index c36642a..3bc6970 100644
    --- a/Makefile
    +++ b/Makefile
    @@ -1,12 +1,15 @@
    -include config.mk

    +MUSL_INC = $HOME/musl/include
    +
    CFLAGS := -Wall -g -O2 \
            -fno-pie -fno-pic -fno-stack-protector \
            -fno-zero-initialized-in-bss \
            -static -fno-builtin -nostdlib -nostdinc -ffreestanding -nostartfiles \
            -mgeneral-regs-only \
            -MMD -MP \
    -		  -Iinc -Ilibc/obj/include -Ilibc/arch/aarch64 -Ilibc/include -Ilibc/arch/generic
    +          -Iinc -I$(MUSL_INC) -I$(MUSL_INC)/sys
    +#		  -Iinc -Ilibc/obj/include -Ilibc/arch/aarch64 -Ilibc/include -Ilibc/arch/generic

    CFLAGS += -DNOT_DEBUG -DLOG_DEBUG -DRASPI=$(RASPI)

    @@ -81,8 +84,8 @@ lint:

    clean:
        $(MAKE) -C usr clean
    -	$(MAKE) -C libc clean
    -	$(MAKE) -C boot clean
    +#	$(MAKE) -C libc clean
    +#	$(MAKE) -C boot clean
        rm -rf $(BUILD_DIR)

    .PHONY: init all lint clean qemu qemu-gdb gdb

    $ vi usr/Makefile
    diff --git a/usr/Makefile b/usr/Makefile
    index c49e33b..da9a1f9 100644
    --- a/usr/Makefile
    +++ b/usr/Makefile
    @@ -1,22 +1,24 @@
    -include ../config.mk

    -LIBC_A = ../libc/lib/libc.a
    -LIBC_SPEC = ../libc/lib/musl-gcc.specs
    -LIBC_LIBS = $(wildcard ../libc/lib/*)
    -LIBC_INCS = $(wildcard ../libc/obj/include/**/*) $(wildcard ../libc/include/**/*)
    +MUSL = $HOME/musl
    +LIBC_A = $(MUSL)/libc/lib/libc.a
    +LIBC_SPEC = $(MUSL)/lib/musl-gcc.specs
    +LIBC_LIBS = $(wildcard $(MUSL)/lib/*)
    +LIBC_INCS = $(wildcard $(MUSL)/include/**/*)
    LIBC_DEPS = $(LIBC_LIBS) $(LIBC_INCS)

    OBJ = ../obj/usr
    -LIBC_SPEC_OUT = $(OBJ)/musl-gcc.specs
    +#LIBC_SPEC_OUT = $(OBJ)/musl-gcc.specs

    -USR_CC := $(CC) -specs $(LIBC_SPEC_OUT)
    +#USR_CC := $(CC) -specs $(LIBC_SPEC_OUT)
    +USR_CC := $(MUSL)/bin/musl-gcc

    # -z max-page-size: https://stackoverflow.com/questions/33005638/how-to-change-alignment-of-code-segment-in-elf
    CFLAGS = -std=gnu99 -O3 -MMD -MP -static -z max-page-size=4096 \
    -  -fno-omit-frame-pointer \
    -  -I../libc/obj/include/ \
    -  -I../libc/arch/aarch64/ \
    -  -I../libc/arch/generic/
    +  -fno-omit-frame-pointer
    +#  -I../libc/obj/include/ \
    +#  -I../libc/arch/aarch64/ \
    +#  -I../libc/arch/generic/

    BIN := $(OBJ)/bin
    SRC := src
    @@ -25,10 +27,10 @@ USER_DIRS := $(shell find $(SRC) -maxdepth 1 -mindepth 1 -type d)
    USER_BINS := $(USER_DIRS:$(SRC)/%=$(BIN)/%)

    all:
    -	$(MAKE) -C ../libc
    -	mkdir -p $(dir $(LIBC_SPEC_OUT))
    +#	$(MAKE) -C ../libc
    +#	mkdir -p $(dir $(LIBC_SPEC_OUT))
        # Replace "/usr/local/musl" to "../libc"
    -	sed -e "s/\/usr\/local\/musl/..\/libc/g" $(LIBC_SPEC) > $(LIBC_SPEC_OUT)
    +#	sed -e "s/\/usr\/local\/musl/..\/libc/g" $(LIBC_SPEC) > $(LIBC_SPEC_OUT)
        $(MAKE) $(USER_BINS)

    $(OBJ)/%.c.o: %.c $(LIBC_DEPS)
    ```

- objdumpで次のようなDwarf Errorが発生する。このobjdumpをコマンドラインで実行してもエラーは発生しない。実行には支障はなさそうなので無視する

    ```
    aarch64-unknown-linux-gnu-objdump -S -d ../obj/usr/bin/cat > ../obj/usr/src/cat/cat.asm
    aarch64-unknown-linux-gnu-objdump: Dwarf Error: Can't find .debug_ranges section.
    aarch64-unknown-linux-gnu-objdump: Dwarf Error: found dwarf version '4097', this reader only handles version 2, 3, 4 and 5 information.
    ```

```
$ rm -rf obj
$ make
$ make qemu
$ ls
.              4000 1 512
..             4000 1 512
cat            8000 2 39104
init           8000 3 23216
bigtest        8000 4 39128
echo           8000 5 39328
mkfs           8000 6 45152
sh             8000 7 52712
utest          8000 8 16752
ls             8000 9 39552
console        0 10 0
$ echo abc
abc
$ echo abc > test
$ cat test
abc
```

## DWarfエラーについて

- gccとbinutilsのバージョンが合わないのが原因らしい
- binutilsのバージョンが古いようだ

```
$ /usr/local/bin/aarch64-unknown-linux-gnu-gcc --version
aarch64-unknown-linux-gnu-gcc (GCC) 11.2.0

$ /usr/local/bin/aarch64-unknown-linux-gnu-ld --version
GNU ld (GNU Binutils) 2.29.1

$ /usr/local/bin/aarch64-unknown-linux-gnu-objdump --version
GNU objdump (GNU Binutils) 2.29.1
```

- brewをaarch64で検索すると本家にaarch64版のgccとbinutilsがありバージョンも新しい

```
$ brew search aarch64
==> Formulae
aarch64-elf-binutils
aarch64-elf-gcc
messense/macos-cross-toolchains/aarch64-unknown-linux-gnu ✔
messense/macos-cross-toolchains/aarch64-unknown-linux-musl
sergiobenitez/osxct/aarch64-none-elf

==> Casks
gcc-aarch64-embedded

$ brew info aarch64-elf-gcc
aarch64-elf-gcc: stable 11.2.0 (bottled)
GNU compiler collection for aarch64-elf

$ brew info aarch64-elf-binutils
aarch64-elf-binutils: stable 2.38 (bottled)
GNU Binutils for aarch64-elf cross development
```

- messense/macos-cross-toolchains/aarch64-unknown-linux-gnuをアンインストールし、aarch64-elf-gccとaarch64-elf-binutilsをインストール

```
$ brew uninstall messense/macos-cross-toolchains/aarch64-unknown-linux-gnu
Uninstalling /usr/local/Cellar/aarch64-unknown-linux-gnu/11.2.0... (3,958 files, 383.2MB)

$ brew install aarch64-elf-binutils
==> Downloading https://ghcr.io/v2/homebrew/core/aarch64-elf-binutils/manifests/2.38
Already downloaded: /Users/dspace/Library/Caches/Homebrew/downloads/449db330764694949129ec8e2c5d9ae3d5ba204a81608e2ff5fc7fff0aaa4255--aarch64-elf-binutils-2.38.bottle_manifest.json
==> Downloading https://ghcr.io/v2/homebrew/core/aarch64-elf-binutils/blobs/sha256:def04262f48a5bc1a5b3a9b1331c7e
Already downloaded: /Users/dspace/Library/Caches/Homebrew/downloads/5e0f79d93c9a44bcdceb32ea8c9172c0a9da4cb8218b025271163cc36f3c15b3--aarch64-elf-binutils--2.38.big_sur.bottle.tar.gz
==> Pouring aarch64-elf-binutils--2.38.big_sur.bottle.tar.gz
🍺  /usr/local/Cellar/aarch64-elf-binutils/2.38: 189 files, 37.5MB
==> Running `brew cleanup aarch64-elf-binutils`...
Disable this behaviour by setting HOMEBREW_NO_INSTALL_CLEANUP.
Hide these hints with HOMEBREW_NO_ENV_HINTS (see `man brew`).

$ aarch64-elf-objdump --version
GNU objdump (GNU Binutils) 2.38
Copyright (C) 2022 Free Software Foundation, Inc.

$ brew install aarch64-elf-gcc
==> Downloading https://ghcr.io/v2/homebrew/core/aarch64-elf-gcc/manifests/11.2.0
Already downloaded: /Users/dspace/Library/Caches/Homebrew/downloads/942e53f7676f7cb47bb370ded88c37eb8f87e98e39c87741ecc3548cb3ebbfd2--aarch64-elf-gcc-11.2.0.bottle_manifest.json
==> Downloading https://ghcr.io/v2/homebrew/core/aarch64-elf-gcc/blobs/sha256:d7c2e86dddb3b3f8eeee5eb1c779d799f81
Already downloaded: /Users/dspace/Library/Caches/Homebrew/downloads/a0716bb94bb11e696a04479ed8bb5be38ca236a3b812dcd3acb5b328c6be4532--aarch64-elf-gcc--11.2.0.big_sur.bottle.tar.gz
==> Pouring aarch64-elf-gcc--11.2.0.big_sur.bottle.tar.gz
🍺  /usr/local/Cellar/aarch64-elf-gcc/11.2.0: 536 files, 129.2MB
==> Running `brew cleanup aarch64-elf-gcc`...
Disable this behaviour by setting HOMEBREW_NO_INSTALL_CLEANUP.
Hide these hints with HOMEBREW_NO_ENV_HINTS (see `man brew`).

$ aarch64-elf-gcc --version
aarch64-elf-gcc (GCC) 11.2.0
Copyright (C) 2021 Free Software Foundation, Inc.
```

## 新しいgccでmuslを再インストール

- makeで`lib/musl-gcc.specs`と`obj/musl-gcc`が作成されない
- 作成後にmake installしてもインストールされない

```
$ cd libc
$ make clean
$ export CROSS_COMPILE=aarch64-elf- && ./configure --target=aarch64 --prefix=/Users/dspace/musl --syslibdir=/Users/dspace/musl/lib
$ make
$ make install
$ make lib/musl-gcc.specs
$ make obj/musl-gcc
$ ./tools/install.sh -D -m 644 lib/musl-gcc.specs $HOME/musl/lib/musl-gcc.specs
$ ./tools/install.sh -D obj/musl-gcc $HOME/musl/bin/musl-gcc
```

## xv6を再コンパイル

- エラー

    ```
    $ rm -rf obj
    $ vi config.mk
        CROSS := aarch64-elf-
    $ make

    /Users/dspace/musl/bin/musl-gcc -std=gnu99 -O3 -MMD -MP -static -z max-page-size=4096 -fno-omit-frame-pointer -c -o ../obj/usr/src/cat/main.c.o src/cat/main.c
    mkdir -p ../obj/usr/bin/
    /Users/dspace/musl/bin/musl-gcc -std=gnu99 -O3 -MMD -MP -static -z max-page-size=4096 -fno-omit-frame-pointer -o ../obj/usr/bin/cat ../obj/usr/src/cat/main.c.o
    /usr/local/opt/aarch64-elf-binutils/bin/aarch64-elf-ld: cannot find crtbeginS.o: No such file or directory
    /usr/local/opt/aarch64-elf-binutils/bin/aarch64-elf-ld: cannot find crtendS.o: No such file or directory
    collect2: error: ld returned 1 exit status
    ```

- crtbeginS.oとcrtendS.oはgccのファイル
- messense/macos-cross-toolchains版ではこのエラーはでなかったので何か考慮されているのかもしれない
  - messense版のgccにはcrtbeginS.o, crtendS.oが含まれていた
  - brew本家版にはこれらがない
  - crtbeginS.oはdynamic link時に、crtbeginT.oはstatic link時にcrtbegin.oの代わりに使われるらしい ([Mini FAQ about the misc libc/gcc crt files](https://dev.gentoo.org/~vapier/crt.txt))
  - objdumpすると本家版のcrtbegin.oはstatic link版に該当するようだった
  - いずれにしてもc++のコンストラクタ、デコンストラクタ関係でありcには無関係か?

    ```
    $ tar tvf aarch64-unknown-linux-gnu-aarch64-darwin.tar.gz | grep crtbegin
    -rw-r--r--  0 messense staff     3328  2 14 21:15 aarch64-unknown-linux-gnu/lib/gcc/aarch64-unknown-linux-gnu/11.2.0/crtbegin.o
    -rw-r--r--  0 messense staff     3448  2 14 21:15 aarch64-unknown-linux-gnu/lib/gcc/aarch64-unknown-linux-gnu/11.2.0/crtbeginS.o
    -rw-r--r--  0 messense staff     3936  2 14 21:15 aarch64-unknown-linux-gnu/lib/gcc/aarch64-unknown-linux-gnu/11.2.0/crtbeginT.o
    $ tar tvf aarch64-unknown-linux-gnu-aarch64-darwin.tar.gz | grep crtend
    -rw-r--r--  0 messense staff     1224  2 14 21:15 aarch64-unknown-linux-gnu/lib/gcc/aarch64-unknown-linux-gnu/11.2.0/crtend.o
    -rw-r--r--  0 messense staff     1224  2 14 21:15 aarch64-unknown-linux-gnu/lib/gcc/aarch64-unknown-linux-gnu/11.2.0/crtendS.o
    $ ls /usr/local/opt/aarch64-elf-gcc/lib/gcc/aarch64-elf/11.2.0/
    crtbegin.o  crtfastmath.o  crtn.o  include        install-tools  libgcov.a
    crtend.o    crti.o         ilp32   include-fixed  libgcc.a       plugin
    ```

- `$HOME/musl/lib/musl-gcc.specs`を編集して当該ファイル指定を削除したらエラーはなくなり、実行も問題なくできた
- DWarfエラーもない
- とりあえず、これで行くことにする

```diff
$ diff -uw musl-gcc.specs.org musl-gcc.specs
--- musl-gcc.specs.org	2022-06-14 19:02:35.000000000 +0900
+++ musl-gcc.specs	2022-06-14 19:03:00.000000000 +0900
@@ -13,10 +13,10 @@
 libgcc.a%s %:if-exists(libgcc_eh.a%s)

 *startfile:
-%{!shared: /Users/dspace/musl/lib/Scrt1.o} /Users/dspace/musl/lib/crti.o crtbeginS.o%s
+%{!shared: /Users/dspace/musl/lib/Scrt1.o} /Users/dspace/musl/lib/crti.o

 *endfile:
-crtendS.o%s /Users/dspace/musl/lib/crtn.o
+/Users/dspace/musl/lib/crtn.o

 *link:
 -dynamic-linker /Users/dspace/musl/lib/ld-musl-aarch64.so.1 -nostdlib %{shared:-shared} %{static:-static} %{rdynamic:-export-dynamic}
```

## usrで-gが指定できない

```
$ ~/musl/bin/musl-gcc -o hello_musl -g hello.c
/usr/local/opt/aarch64-elf-binutils/bin/aarch64-elf-ld: cannot find -lg: No such file or directory
collect2: error: ld returned 1 exit status
```

### debugを有効にするオプションがあった

- `--enable-debug          build with debugging information [disabled]`

### muslを再構築

```
$ cd musl
$ make clean
$ export CROSS_COMPILE=aarch64-elf- && ./configure --target=aarch64 --prefix=$(HOME)/musl --syslibdir=$(HOME)/musl/lib --enable-debug
$ make
$ make install
$ make lib/musl-gcc.specs
$ diff -uw $(HOME)/musl/lib/musl-gcc.specs musl-gcc.specs
--- /Users/dspace/musl/lib/musl-gcc.specs	2022-06-14 19:03:00.000000000 +0900
+++ musl-gcc.specs	2022-07-07 09:58:16.000000000 +0900
@@ -13,10 +13,10 @@
 libgcc.a%s %:if-exists(libgcc_eh.a%s)

 *startfile:
-%{!shared: /Users/dspace/musl/lib/Scrt1.o} /Users/dspace/musl/lib/crti.o
+%{!shared: /Users/dspace/musl/lib/Scrt1.o} /Users/dspace/musl/lib/crti.o crtbeginS.o%s

 *endfile:
-/Users/dspace/musl/lib/crtn.o
+crtendS.o%s /Users/dspace/musl/lib/crtn.o

 *link:
 -dynamic-linker /Users/dspace/musl/lib/ld-musl-aarch64.so.1 -nostdlib %{shared:-shared} %{static:-static} %{rdynamic:-export-dynamic}
$ touch $(HOME)/musl/lib/musl-gcc.specs
$ make obj/musl-gcc
$ diff -uw musl-gcc ~/musl/bin/musl-gcc
$ touch $(HOME)/musl/bin/musl-gcc
```

### coreutils, dash, xv6を再構築

```
$ cd $COREUTILS
$ make clean
$ CC=$HOME/musl/bin/musl-gcc ./configure --host=aarch64-elf CFLAGS="-std=gnu99 -O3 -MMD -MP -static -fno-plt -fno-pic -fpie -z max-page-size=4096"
$ make
$ cd src
$ file ls
ls: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, with debug_info, not stripped
$ find . -type f -perm +0111 -not -name 'make-prime-list' -not -name '*.so' -exec cp {} ../../usr/bin \;

$ cd $DASH
$ make clean
$ CC=/Users/dspace/musl/bin/musl-gcc ./configure --host=aarch64-elf --enable-static CFLAGS="-std=gnu99 -O3 -MMD -MP -static -fno-plt -fno-pic -fpie -z max-page-size=4096 -Isrc"
$ make
$ cp src/dash ../usr/bin

$ cd $XV6
$ rm -rf obj
$ make
```

libcのコードにデバッグ情報を追加するものでmusl-gccが`-g`に対応できるようにするものではなかったので、戻した。

```
$ ~/musl/bin/musl-gcc -static -g -o hello-musl hello.c
/usr/local/opt/aarch64-elf-binutils/bin/aarch64-elf-ld: cannot find -lg: No such file or directory
collect2: error: ld returned 1 exit status
$ ~/musl/bin/musl-gcc -static -o hello-musl hello.c
$ file hello-musl
hello-musl: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, with debug_info, not stripped
$ aarch64-elf-objdump -S -d hello-musl
...
0000000000400050 <_start_c>:
    void (*)(), void(*)(), void(*)());

void _start_c(long *p)
{
    int argc = p[0];
    char **argv = (void *)(p+1);
  400050:   aa0003e2    mov x2, x0
    __libc_start_main(main, argc, argv, _init, _fini, 0);
  400054:   b0000084    adrp    x4, 411000 <__stdio_ofl_lockptr+0xfdb8>
  400058:   b0000083    adrp    x3, 411000 <__stdio_ofl_lockptr+0xfdb8>
  40005c:   b0000080    adrp    x0, 411000 <__stdio_ofl_lockptr+0xfdb8>
  400060:   f9413484    ldr x4, [x4, #616]
  400064:   d2800005    mov x5, #0x0                    // #0
  400068:   f9412c63    ldr x3, [x3, #600]
  40006c:   f9413000    ldr x0, [x0, #608]
    int argc = p[0];
  400070:   f8408441    ldr x1, [x2], #8
    __libc_start_main(main, argc, argv, _init, _fini, 0);
  400074:   14000093    b   4002c0 <__libc_start_main>

0000000000400078 <main>:
  400078:   a9bf7bfd    stp x29, x30, [sp, #-16]!
  40007c:   910003fd    mov x29, sp
  400080:   b0000000    adrp    x0, 401000 <__stdio_write+0x90>
  400084:   91080000    add x0, x0, #0x200
  400088:   940000b3    bl  400354 <puts>
  40008c:   52800000    mov w0, #0x0                    // #0
  400090:   a8c17bfd    ldp x29, x30, [sp], #16
  400094:   d65f03c0    ret
```

### ファイルの大きさ

- `--enable-debug`

```
$ ls -l usr/bin/dash usr/bin/ls
-rwxr-xr-x 1 dspace staff 765184  7  7 10:13 usr/bin/dash
-rwxr-xr-x 1 dspace staff 901112  7  7 10:10 usr/bin/ls
```

- `--disable-debug`

```
$ ls -l usr/bin/ls usr/bin/dash
-rwxr-xr-x 1 dspace staff 332056  7  7 16:01 usr/bin/dash
-rwxr-xr-x 1 dspace staff 439864  7  7 16:03 usr/bin/ls
```
