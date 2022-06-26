#ifndef	INC_LINUX_UTSNAME_H
#define	INC_LINUX_UTSNAME_H

#define UNAME_SYSNAME "xv6"
#define UNAME_NODENAME "mini"
#define UNAME_RELEASE "1.0.1"
#define UNAME_VERSION "2022-06-26 (musl)"
#define UNAME_MACHINE "AArch64"

struct utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char __domainname[65];
};

#endif
