#ifndef USR_INC_FILES_H
#define USR_INC_FILES_H

char *usrbins[] = {
    "usr/bin/tee",
    "usr/bin/md5sum",
    "usr/bin/split",
    "usr/bin/cat",
    "usr/bin/shuf",
    "usr/bin/mkfifo",
    "usr/bin/pathchk",
    "usr/bin/runcon",
    "usr/bin/expand",
    "usr/bin/tty",
    "usr/bin/basename",
    "usr/bin/nice",
    "usr/bin/truncate",
    "usr/bin/echo",
    "usr/bin/du",
    "usr/bin/ptx",
    "usr/bin/join",
    "usr/bin/df",
    "usr/bin/pwd",
    "usr/bin/test",
    "usr/bin/csplit",
    "usr/bin/sort",
    "usr/bin/whoami",
    "usr/bin/touch",
    "usr/bin/dcgen",
    "usr/bin/unlink",
    "usr/bin/b2sum",
    "usr/bin/sleep",
    "usr/bin/fmt",
    "usr/bin/stty",
    "usr/bin/logname",
    "usr/bin/chgrp",
    "usr/bin/printenv",
    "usr/bin/seq",
    "usr/bin/uname",
    "usr/bin/sha224sum",
    "usr/bin/od",
    "usr/bin/date",
    "usr/bin/base64",
    "usr/bin/realpath",
    "usr/bin/readlink",
    "usr/bin/dircolors",
    "usr/bin/timeout",
    "usr/bin/tac",
    "usr/bin/numfmt",
    "usr/bin/wc",
    "usr/bin/basenc",
    "usr/bin/comm",
    "usr/bin/nproc",
    "usr/bin/expr",
    "usr/bin/stdbuf",
    "usr/bin/cksum",
    "usr/bin/printf",
    "usr/bin/groups",
    "usr/bin/chcon",
    "usr/bin/factor",
    "usr/bin/tail",
    "usr/bin/env",
    "usr/bin/pr",
    "usr/bin/head",
    "usr/bin/kill",
    "usr/bin/uniq",
    "usr/bin/stat",
    "usr/bin/link",
    "usr/bin/sum",
    "usr/bin/tsort",
    "usr/bin/mknod",
    "usr/bin/users",
    "usr/bin/dd",
    "usr/bin/who",
    "usr/bin/sha1sum",
    "usr/bin/mktemp",
    "usr/bin/cut",
    "usr/bin/sha256sum",
    "usr/bin/dir",
    "usr/bin/mkdir",
    "usr/bin/nl",
    "usr/bin/ginstall",
    "usr/bin/shred",
    "usr/bin/fold",
    "usr/bin/rmdir",
    "usr/bin/sha384sum",
    "usr/bin/mv",
    "usr/bin/dirname",
    "usr/bin/id",
    "usr/bin/base32",
    "usr/bin/pinky",
    "usr/bin/ln",
    "usr/bin/hostid",
    "usr/bin/chroot",
    "usr/bin/ls",
    "usr/bin/true",
    "usr/bin/cp",
    "usr/bin/sync",
    "usr/bin/yes",
    "usr/bin/unexpand",
    "usr/bin/chown",
    "usr/bin/getlimits",
    "usr/bin/chmod",
    "usr/bin/uptime",
    "usr/bin/rm",
    "usr/bin/vdir",
    "usr/bin/false",
    "usr/bin/sha512sum",
    "usr/bin/[",
    "usr/bin/tr",
    "usr/bin/paste",
    "usr/bin/nohup",
    "usr/bin/dash",
    NULL
};

char *etc_files[] = {
    "usr/etc/passwd",
    "usr/etc/group",
    "usr/etc/inittab",
    NULL
};

char *lib_files[] = {
    "usr/lib/libc.so",
    NULL
};

char *text_files[] = {
    "usr/test.txt",
    NULL
};

int nelms(char **ary) {
    int num = 0;
    for (char **p = ary; *p; p++) {
        num++;
    }
    return num;
}

#endif
