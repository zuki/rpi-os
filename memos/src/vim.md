# ncurses6.2

## with-sharedはconfigureでエラー

```
$ CC=$(HOME)/musl/bin/musl-gcc CFLAGS="-I$(HOME)/musl/include -O3 -fpie" LDFLAGS="-L$(HOME)/musl/lib" ./configure --host=aarch64-elf --prefix=$(HOME)/gnu/ncurses --with-shared
...
checking which $(HOME)/musl/bin/musl-gcc option to use... -fPIC
configure: error: Shared libraries are not supported in this version
```

## staticライブラリのみ作成

- コマンドとライブラリの作成は成功
- `make install`で一部のterminfoのコンパイルに失敗

```
$ mkdir -p gnu/ncurses
$ cd NCURSES
$ CC=$(HOME)/musl/bin/musl-gcc CFLAGS="-I$(HOME)/musl/include -O3" LDFLAGS="-L$(HOME)/musl/lib" ./configure --host=aarch64-elf --prefix=$(HOME)/gnu/ncurses --without-cxx
$ make
$ ls -l lib
total 7284
-rw-r--r-- 1 dspace staff  122772  8 16 09:46 libform.a
-rw-r--r-- 1 dspace staff  978026  8 16 09:46 libform_g.a
-rw-r--r-- 1 dspace staff   59822  8 16 09:46 libmenu.a
-rw-r--r-- 1 dspace staff  489772  8 16 09:46 libmenu_g.a
-rw-r--r-- 1 dspace staff  687644  8 16 09:45 libncurses.a
-rw-r--r-- 1 dspace staff 4783482  8 16 09:46 libncurses_g.a
-rw-r--r-- 1 dspace staff   23138  8 16 09:46 libpanel.a
-rw-r--r-- 1 dspace staff  304560  8 16 09:46 libpanel_g.a
$ make install
...
** Building terminfo database, please wait...
Running tic to install $(HOME)/gnu/ncurses/share/terminfo ...

	You may see messages regarding extended capabilities, e.g., AX.
	These are extended terminal capabilities which are compiled
	using
		tic -x
	If you have ncurses 4.2 applications, you should read the INSTALL
	document, and install the terminfo without the -x option.

"terminfo.tmp", line 1635, terminal 'pccon+base': enter_bold_mode but no exit_attribute_mode
"terminfo.tmp", line 1635, terminal 'pccon+base': enter_reverse_mode but no exit_attribute_mode
"terminfo.tmp", line 4790, terminal 'xterm-16color': error writing $(HOME)/gnu/ncurses/share/terminfo/78/xterm-16color
? tic could not build $(HOME)/gnu/ncurses/share/terminfo
make[1]: *** [install.data] Error 1
make: *** [install] Error 2
$ $(HOME)/gnu/ncurses
bin  include  lib  share
$ ls $(HOME)/gnu/ncurses/bin
captoinfo  clear  infocmp  infotocap  reset  tabs  tic  toe  tput  tset
$ ls $(HOME)/gnu/ncurses/lib
libform.a    libmenu.a    libncurses.a    libpanel.a
libform_g.a  libmenu_g.a  libncurses_g.a  libpanel_g.a
$ ls $(HOME)/gnu/ncurses/include/ncurses/
curses.h  form.h  nc_tparm.h  ncurses_dll.h  term.h        termcap.h  unctrl.h
eti.h     menu.h  ncurses.h   panel.h        term_entry.h  tic.h
$ ls $(HOME)/gnu/ncurses/share/
man  tabset  terminfo
$ ls $(HOME)/gnu/ncurses/share/terminfo
39  61  63  65  67  69  6b  6d  6f  71  73  75  77  7a
41  62  64  66  68  6a  6c  6e  70  72  74  76  78
2$ ls $(HOME)/gnu/ncurses/share/terminfo/76
v200-nam       vt100-am     vt100-w-nam  vt200-old     vt300-w-nam  vt420pcdos
v320n          vt100-bot-s  vt100-w-nav  vt200-w       vt320        vt510
vanilla        vt100-nam    vt100nam     vt220         vt320-nam    vt510pc
vs100-x10      vt100-nam-w  vt102        vt220+keypad  vt320-w      vt510pcdos
vscode         vt100-nav    vt102+enq    vt220-8       vt320-w-nam  vt52
vscode-direct  vt100-nav-w  vt102-nsgr   vt220-8bit    vt320nam     vt520
vt-utf8        vt100-putty  vt102-w      vt220-js      vt330        vt520ansi
vt100          vt100-s      vt125        vt220-nam     vt340        vt525
vt100+         vt100-s-bot  vt131        vt220-old     vt400        vtnt
vt100+4bsd     vt100-s-top  vt132        vt220-w       vt400-24
vt100+enq      vt100-top-s  vt200        vt220d        vt420
vt100+fnkeys   vt100-vb     vt200-8      vt300         vt420+lrmm
vt100+keypad   vt100-w      vt200-8bit   vt300-nam     vt420f
vt100+pfkeys   vt100-w-am   vt200-js     vt300-w       vt420pc
```

# vim

- ncursesがリンクできずconfigureがエラー
- [cross compile vim: --with-tlib #2058](https://github.com/vim/vim/issues/2058)を参考に各種環境変数を設定したが解消せず

```
$ export vim_cv_toupper_broken=set
$ export vim_cv_terminfo=yes
$ export vim_cv_tgetent=zero
$ CC=$(HOME)/musl/bin/musl-gcc LDFLAGS="-L$(HOME)/musl/lib -L$(HOME)/gnu/ncurses/lib" ./configure --host=aarch64-elf --prefix=$(HOME)/gnu/vim --with-features=tiny --disable-gui --without-x --disable-nls --with-tlib=ncurses
(...)
checking --with-tlib argument... ncurses
checking for linking with ncurses library... configure: error: FAILED
```

# viを導入

- [nyuichi/xv6](https://github.com/nyuichi/xv6)のviコマンドを導入

```
$ mkdir -p usr/src/vi
$ cp NYUICHI_XV6/usr/vi.c usr/src/vi/main.c
$ cp NYUICHI_XV6/lib/curses.c usr/src/vi/
$ cp NYUICHI_XV6/include/curses.h usr/inc/
$ vi XV6/usr/src/vi/main.c
-#define SCREEN_WIDTH  30
-#define SCREEN_HEIGHT 20
+#define SCREEN_WIDTH 80
+#define SCREEN_HEIGHT 24

-struct termios termios;
+extern struct termios termios;

$ vi XV6/usr/src/vi/curses.c
-#include <curses.h>
+#include "curses.h"

-ScreenType screen;
+ScreenType screenbuf;

-screen[i][j].letter = ' ';
-screen[i][j].color = 0;
+screenbuf[i][j].letter = ' ';
+screenbuf[i][j].color = 0;

-screen[cursolY][cursolX].letter = c;
+screenbuf[cursolY][cursolX].letter = c;

-screen[cursolY][cursolX].letter = str[i];
+screenbuf[cursolY][cursolX].letter = str[i];

-buf[i*(COLS+1) + j] = screen[i][j].letter;
+buf[i * (COLS + 1) + j] = screenbuf[i][j].letter;

$ vi XV6/usr/inc/curses.h
-#ifndef _CURSES_H
-#define _CURSES_H
+#ifndef USR_INC_CURSES_H
+#define USR_INC_CURSES_H

$ make qemu
Welcome to xv6 2022-06-26 (musl) mini tty

mini login: zuki
Password:
[2]fileopen: cant namei /etc/profile
[2]fileopen: cant namei /.profile
$ ls
ch08
$ vi
==========================================		// ここから
vi test
123 abc
vi test
123 abc
jj
~
~
~
~
~
~
~
~
~
~
~
~
~
~
~
~
~
~
~
[normal]  :w test.tx							// ここはコマンド入力行
==========================================		// ここまでを入力があるごとに再描画
[normal]  :q
$ ls
ch08  test.txt
$ cat test.txt
vi test
123 abc
vi test
123 abc
jj
$
```

- 文字入力があるごとにすべての行を再描画するので画面がチラチラする
- とりあえず入力はできるのでcat > test.txtよりはるかに便利

## 更新履歴

```
$ git status
On branch mac
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	modified:   memos/src/SUMMARY.md
	modified:   memos/src/usb.md
	new file:   memos/src/vim.md
	new file:   usr/inc/curses.h
	new file:   usr/src/vi/curses.c
	new file:   usr/src/vi/main.c
```
