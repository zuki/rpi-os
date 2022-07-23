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

### 課題2 解決

1. exec.cでSet-uid Set-gidの処理を追加
2. fs.c$permission()を修正
3. /etc/passwdは更新されセッション中は新パスワードが有効であるが、qemuを再実行すると`usr/etc/passwd`が使用され新パスワードは破棄される。新パスワードを有効にするにはusr/etc/passwdを変更する必要がある

#### 解決後の実行ログ

```
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
[3]fileopen: cant namei /etc/profile
[3]fileopen: cant namei /.profile
$ passwd zuki
Old password:
New password:
Retype new password:
$
```

#### 解決策(1)の実施

- 変更前: /etc/passwdを再作成できない

```
[3]sys_openat: dirfd -100, path '/etc/ptmp', flag 0x20241, mode0x180
[3]permission: ip: mode=0x00008180, uid=1000, gid=1000, p: fsuid=1000, fsgid=0, mask: 0x00000002
[1]sys_openat: dirfd -100, path '/etc/ptmp', flag 0x20241, mode0x1b6
[1]permission: ip: mode=0x00008180, uid=1000, gid=1000, p: fsuid=1000, fsgid=0, mask: 0x00000002
[3]sys_openat: dirfd -100, path '/etc/passwd', flag 0x20000, mode0x1b6
[3]permission: ip: mode=0x000081a4, uid=0, gid=0, p: fsuid=1000, fsgid=0, mask: 0x00000004
[2]sys_openat: dirfd -100, path '/etc/ptmp', flag 0x20000, mode0x0
[2]permission: ip: mode=0x00008180, uid=1000, gid=1000, p: fsuid=1000, fsgid=0, mask: 0x00000004
[1]sys_openat: dirfd -100, path '/etc/passwd', flag 0x20241, mode0x1a4
[1]permission: ip: mode=0x000081a4, uid=0, gid=0, p: fsuid=1000, fsgid=0, mask: 0x00000002
[Cannot recreat passwd file.
```

- 変更後: 一時ファイルに書き込めない

```
Retype new password:
[2]sys_openat: dirfd -100, path '/etc/ptmp', flag 0x20241, mode0x180
[2]permission: ip: mode=0x00008180, uid=1000, gid=1000, p: fsuid=0, fsgid=0, mask: 0x00000002
[2]sys_openat: dirfd -100, path '/etc/ptmp', flag 0x20241, mode0x1b6
[2]permission: ip: mode=0x00008180, uid=1000, gid=1000, p: fsuid=0, fsgid=0, mask: 0x00000002
Cannot create temporary file
```

#### fsuidがrootなら無条件で許可

```diff
$ git diff kern/fs.c
@@ -937,10 +937,11 @@ permission(struct inode *ip, int mask)
 {
     struct proc *p = thisproc();
     mode_t mode = ip->mode;

+    if (p->fsuid == 0) return 0;
+
     if (p->fsuid == ip->uid)
         mode >>= 6;
     else if (p->fsgid == ip->gid)
         mode >>= 3;
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

### 問題2-1: loginでパスワードを間違えると再ログインできない

```
mini login: zuki
Password:             // パスワードを間違えると
Login incorrect       // この表示のあとストール
```

#### 解決2-1: 再ログインを促すプロンプトがflushされていなかった

```
mini login: zuki
user: zuki
Password:
Login incorrect
login: zuki                 // プロンプトが出るようになった
Password:                   // 正しいパスワードを入れてもincorrectになる
Login incorrect
login:
```

### 問題2-2: 再ログイン時に正しいパスワードを入力してもincorrectになる

```
login: zuki
Password:
namep: 'CyFEE3.../.2.', pwd: 'Cy33E..I//3/    // 入力するたびに別のcryptになる
InLogin incorrect
login: zuki
Password:
namep: 'Cy3FI.3/22EFE', pwd: 'Cy33E..I//3/    //
InLogin incorrect
login:
```

#### 解決2-2; muslのcrypt()を使うようにした

- v7版crypt(pw, salt)はpw, salt共に同じ値を指定しても連続して呼び出すとその度に違う結果となる
- 作業配列をstatic arrayで確保しているが、これを実行毎に初期化していないので前回の実行結果が残っていて結果に影響するのではないかと思われる
  - 以下の結果で初期化後のblock変数は両者で同じだが、初期化していない作業配列を使ったencrypt後のblockの値が変化している
- musl版crypt()を使用するよう変更したところ、問題なく実行できて、上記の問題もなかったのでこれを使うことにする

```
// 正しく判定された場合
crypt: pw='password', salt='Cy33E..I//3/I'
block: 010101010001000001010100010001000101000100010100010100010000010000010100000100000001010000010000000101000001010000000000000000000000
encry: 000000010001000000010001000100000000000000000000000000000000000100010000000000000001000000000001000000010001000000000001000100010000
namep: 'Cy33E..I//3/I', passwd: 'Cy33E..I//3/I'

// 正しく判定されない場合
crypt: pw='password', salt='Cy33E..I//3/I'
block: 010101010001000001010100010001000101000100010100010100010000010000010100000100000001010000010000000101000001010000000000000000000000
encry: 000000010001000100000001000100010000000000000000000000010001000000000001000000010000000000010000000100000000000100000001000100000000
namep: 'Cy3FI.3/22EFE', passwd: 'Cy33E..I//3/I'
```

```
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
Login incorrect
login: zuki
Password:                                       // パスワード再入力
[0]fileopen: cant namei /etc/profile
[3]fileopen: cant namei /.profile
$                                               // ログイン成功
```

## musl版crypt()は、saltの先頭数バイトも見てハッシュ化の方法を判定している

```c
// libc/src/crypt/crypt_r.c
char *__crypt_r(const char *key, const char *salt, struct crypt_data *data)
...
    if (salt[0] == '$' && salt[1] && salt[2]) {
        if (salt[1] == '1' && salt[2] == '$')
            return __crypt_md5(key, salt, output);
        if (salt[1] == '2' && salt[3] == '$')
            return __crypt_blowfish(key, salt, output);
        if (salt[1] == '5' && salt[2] == '$')
            return __crypt_sha256(key, salt, output);
        if (salt[1] == '6' && salt[2] == '$')
            return __crypt_sha512(key, salt, output);
    }
    return __crypt_des(key, salt, output);
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
