# 10: 各種プロセスIDを導入

- pgid, sidを追加
- wait4()のwastatusとoptionsを実装

## システムコールを追加

- sys_setpgid
- sys_getpgid
- sys_getppid

## 変更履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	new file:   inc/linux/resources.h
	new file:   inc/linux/wait.h
	modified:   inc/proc.h
	modified:   inc/syscall1.h
	modified:   kern/proc.c
	modified:   kern/syscall.c
	modified:   kern/sysproc.c
	modified:   memos/src/SUMMARY.md
	new file:   memos/src/process.md
	modified:   memos/src/user.md
```
