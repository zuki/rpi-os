# ユーザを導入

## プロセス、inodeにユーザ情報を追加

### プロセス

- uid_t uid, euid, suid, fsuid
- gid_t gid, egid, sgid, fsgid
- gid_t groups[NGROUPS]
- int ngroups;
- kernel_cap_t cap_effective, cap_inheritable, cap_permitted
- mode_t umask

### inode

- uid_t uid
- gid_t gid

## システムコールの追加

- sys_fchmodat
- sys_fchownat
- sys_fchown,
- sys_setregid,
- sys_setgid
- sys_setreuid
- sys_setuid
- sys_setresuid
- sys_getresuid
- sys_setresgid
- sys_getresgid
- sys_setfsuid
- sys_setfsgid
- sys_getgroups
- sys_setgroups
- sys_getuid
- sys_geteuid
- sys_getgid
- sys_getegid

## 検証用lsにユーザ情報を追加

```
$$ /bin/ls
drwxrwxr-x    1 root wheel   512  6 25 12:12 .
drwxrwxr-x    1 root wheel   512  6 25 12:12 ..
drwxrwxr-x    2 root wheel   896  6 25 12:12 bin
drwxrwxr-x    3 root wheel   384  6 25 12:12 dev
drwxrwxr-x    8 root wheel   128  6 25 12:12 etc
drwxrwxrwx    9 root wheel   128  6 25 12:12 lib
drwxrwxr-x   10 root wheel   192  6 25 12:12 home
drwxrwxr-x   12 root wheel   192  6 25 12:12 usre
$ /bin/ls /bin
drwxrwxr-x    2 root wheel   896  6 25 12:12 .
drwxrwxr-x    1 root wheel   512  6 25 12:12 ..
-rwxr-xr-x   15 root wheel 38568  6 25 12:12 cat
-rwxr-xr-x   16 root wheel 22448  6 25 12:12 init
-rwxr-xr-x   17 root wheel 39816  6 25 12:12 bigtest
-rwxr-xr-x   18 root wheel 39480  6 25 12:12 echoest
-rwxr-xr-x   19 root wheel 48888  6 25 12:12 mkfsest
-rwxr-xr-x   20 root wheel 48552  6 25 12:12 dateest
-rwxr-xr-x   21 root wheel 54056  6 25 12:12 shteest
-rwxr-xr-x   22 root wheel 22104  6 25 12:12 sigtest3
-rwxr-xr-x   23 root wheel 48640  6 25 12:12 sigtest2
-rwxr-xr-x   24 root wheel 17744  6 25 12:12 utestst2
-rwxr-xr-x   25 root wheel 45040  6 25 12:12 sigtest2
-rwxr-xr-x   26 root wheel 53128  6 25 12:12 lsgtest2
$ /bin/ls /home
drwxrwxr-x   10 root wheel   192  6 25 12:12 .
drwxrwxr-x    1 root wheel   512  6 25 12:12 ..
drwxrwxr-x   11 zuki staff   128  6 25 12:12 zuki
```

## 変更履歴

```
$ git status
On branch mac
	modified:   inc/file.h
	modified:   inc/fs.h
	new file:   inc/linux/capability.h
	modified:   inc/proc.h
	modified:   inc/types.h
	modified:   kern/file.c
	modified:   kern/fs.c
	modified:   kern/proc.c
	modified:   kern/syscall.c
	modified:   kern/sysfile.c
	modified:   kern/sysproc.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/user.md
	modified:   usr/inc/fs.h
	modified:   usr/inc/types.h
	modified:   usr/src/ls/main.c
	modified:   usr/src/mkfs/main.c
```
