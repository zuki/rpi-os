# su, login, passwdコマンドを追加

## suコマンド

- Robert Nordier氏の[v7/x86](https://www.nordier.com/)経由で
- unix version7の`usr/src/cmd/su.c`から流用

### 問題1: zuki -> root でuid, gidが変わらない

```
# su zuki
su before: uid: 0, gid: 0
su after : uid: 1000, gid: 1000
$ pwd
/
$ whoami
zuki
$ su root
su before: uid: 1000, gid: 1000
su after : uid: 1000, gid: 1000
# whoami
root
#
```

## passwd

- Robert Nordier氏の[v7/x86](https://www.nordier.com/)経由で
- unix version7の`usr/src/cmd/passwd.c`から流用

### 問題1: パスワードが表示される

```
# passwd zuki

[1]sys_ioctl: fd=10, req=0x5410, type=2
[1]sys_ioctl: fd=3, req=0x5401, type=2
[1]sys_ioctl: term: 0xfffffffffe90, devsw[1]: 0xffff00003b3ff000
[1]sys_ioctl: TCGETS ok
[1]sys_ioctl: fd=3, req=0x5404, type=2
[3]sys_ioctl: iflag: 0x2d00, oflag: 0x5, cflag: 0x10b2, lflag: 0x8a12
031c7f150400010000001a000000170000000000000000000000000000000000
[1]sys_ioctl: fd=3, req=0x5409, type=2
New password:
password123                                 // パスワードが表示
[3]sys_ioctl: fd=3, req=0x5404, type=2
[3]sys_ioctl: iflag: 0x2d00, oflag: 0x5, cflag: 0x10b2, lflag: 0x8a1b
031c7f150400010000001a000000170000000000000000000000000000000000

[3]sys_ioctl: fd=3, req=0x5401, type=2
[3]sys_ioctl: term: 0xfffffffffe90, devsw[1]: 0xffff00003b3ff000
[3]sys_ioctl: TCGETS ok
[3]sys_ioctl: fd=3, req=0x5404, type=2
[3]sys_ioctl: iflag: 0x2d00, oflag: 0x5, cflag: 0x10b2, lflag: 0x8a12
031c7f150400010000001a000000170000000000000000000000000000000000

[3]sys_ioctl: fd=3, req=0x5409, type=2
Retype new password:
password123                                 // パスワードが表示
[2]sys_ioctl: fd=3, req=0x5404, type=2
[3]sys_ioctl: iflag: 0x2d00, oflag: 0x5, cflag: 0x10b2, lflag: 0x8a1b
031c7f150400010000001a000000170000000000000000000000000000000000

[2]syscall1: proc[7] sys_getpid called
QEMU: Terminated
```

- iflag = 0x2d00 : IMAXBEL + IANY + IXON * ICRNL
- oflag = 0x5    : OPOST + ONLCR
- cflag = 0x10b2 : CBAUDEX + CREAD + CS8 + ?
- lflag = 0x8a12 : IEXTEN + ECHOKE + ECHOPRT + ECHOE + ICANON
- lflag = 0x8a1b : IEXTEN + ECHOKE + ECHOPRT + ECHOE + ICANON + ECHO
- c_cc  = 03 1c 7f 15 04 00 01 00 00 00 1a 00 00 00 17 00 00
  - VINTR  : 0x03
  - VQUIT  : 0x1c
  - VERASE : 0x7f
  - VKILL  : 0x15
  - VEOF   : 0x04
  - VTIME  : 0x00
  - VMIN   : 0x01
  - VSWTC  : 0x00
  - VSTART : 0x00
  - VSTOP  : 0x00
  - VSUSP  : 0x1a
  - VEOL   : 0x00
  - VREPRINT : 0x00
  - VDISCARD : 0x00
  - VWERASE  : 0x17
  - VLNEXT   : 0x00
  - VEOL2    : 0x00

#### 問題1解決

- パスワードが表示されるのはconsole.cでtermiosを反映していなかったため
- 途中でとまるのはpasswd.cのバグだった

```
# passwd zuki
New password:
Retype new password:
# cat /etc/passwd
root::0:0:root:/:/usr/bin/dash
zuki:0yRewojthvtQ2:1000:1000:,,,:/home/zuki:/usr/bin/dash
```

### 課題2: zukiユーザでpasswd変更ができない

```
$ passwd zuki
New password:
Retype new password:
Cannot recreat passwd file.

# chmod 04755 /bin/passwd
# ls -l /bin/passwd
-rwsr-xr-x 1 root root 94856 Jul 21  2022 /bin/passwd
# su zuki
su before: uid: 0, gid: 0
su after : uid: 1000, gid: 1000
$ passwd zuki
New password:
Retype new password:
Cannot recreat passwd file.
```

## login

- Robert Nordier氏の[v7/x86](https://www.nordier.com/)経由で
- unix version7の`usr/src/cmd/login.c`から流用

## getty

- [troglobit/getty](https://github.com/troglobit/getty)を使用

```
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
[3]fileopen: cant namei /etc/profile
[1]fileopen: cant namei /.profile
$ pwd
/home/zuki
$
```

### 問題1: passwdで設定したパスワードでログインできない

```
# cat /etc/passwd
root:3y45VZlZRpDh2:0:0:root:/:/usr/bin/dash

mini login: root
Password:
Login incorrect
```

### 問題1 解決

- loginとpasswdで使用するcrypt関数が違っていた（loginは自前の、passwdはmsulのcrypt。ちなみにsuもmsulのcryptを使っていた）
- usr内でlibraryを作るのが正式だろうが、とりあえずシンボリックリンクで解決

```
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
$ pwd
/home/zuki
$ whoami
zuki
$ su root
Password:
# whoami
root
# su zuki
Password:
$ whoami
zuki
```

## passwdコマンドで"Temporary file busy; try again later"と言われる場合

- `/etc/ptmp`を削除する

## `/etc/inittab`

- initから`dash /etc/initab`を呼び出し、初期化処理をする

```
$ cat etc/inittab
chmod 04755 /bin/passwd
/bin/getty
```

```diff
$ git diff usr/src/init/
diff --git a/usr/src/init/main.c b/usr/src/init/main.c
index c21b8f0..1ac3984 100644
--- a/usr/src/init/main.c
+++ b/usr/src/init/main.c
@@ -7,7 +7,7 @@
 #include <sys/stat.h>
 #include <sys/sysmacros.h>

-char *argv[] = { "dash", 0 };
+char *argv[] = { "dash", "/etc/inittab", 0 };
 //char *envp[] = { "TEST_ENV=FROM_INIT", "TZ=JST-9", 0 };
 char *envp[] = { "TZ=JST-9", 0 };

@@ -33,8 +33,8 @@ main()
         }
         if (pid == 0) {
             //execve("/bin/sh", argv, envp);
-            //execve("/usr/bin/dash", argv, envp);
-            execve("/bin/getty", 0, 0);
+            execve("/usr/bin/dash", argv, envp);
+            //execve("/bin/getty", 0, 0);
             printf("init: exec sh failed\n");
             exit(1);
         }
```

### 実行

```
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: root
Password:
[2]fileopen: cant namei /etc/profile
[1]fileopen: cant namei /.profile
# ls -l /bin/passwd
-rwsr-xr-x 1 root root 66608 Jul 22  2022 /bin/passwd
#
```

## dashをexit可能に

```
mini login: root
Password:
# ls -l /bin/passwd
-rwsr-xr-x 1 root root 66608 Jul 22  2022 /bin/passwd
# exit

Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
$ exit

Welcome to xv6 2022-06-26 (musl) mini tty

mini login:
```
