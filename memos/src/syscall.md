# 11: システテムコールの追加

## 未実装のシステムコールを追加

- sys_getcwd
- sys_faccessat
- sys_faccessat2
- sys_fdatasync
- sys_sched_getaffinity
- sys_uname
- sys_sysinfo
- sys_prlimit64
- sys_renameat2
- sys_getrandom

## 変更履歴

```
$ git status
On branch mac
	modified:   inc/arm.h
	modified:   inc/clock.h
	modified:   inc/file.h
	new file:   inc/linux/sysinfo.h
	new file:   inc/linux/utsname.h
	modified:   inc/mm.h
	modified:   inc/proc.h
	new file:   inc/random.h
	modified:   inc/string.h
	modified:   inc/syscall1.h
	modified:   kern/clock.c
	modified:   kern/file.c
	modified:   kern/fs.c
	modified:   kern/mm.c
	modified:   kern/proc.c
	new file:   kern/random.c
	modified:   kern/syscall.c
	modified:   kern/sysfile.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/syscall.md
```
