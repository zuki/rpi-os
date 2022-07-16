# 13.3 パイプ処理でストール

```
# sort test.txt
111
111
123
123
12345
999
999
あいうえお
あいうえお
あいうえお
かきくけこ
# sort test.txt | uniq				// ストール

# head -5 test.txt
123
111
111
123
999
# cat test.txt | head -5
123
111
111
123
999
# cat test.txt | uniq
123
111
123
999
あいうえお
999
かきくけこ
12345
あいうえお
# cat test.txt | uniq | head -3		// ストール
```

- debug print

```
# cat test.txt | head -5
pid 6, evaltree(0x42e678: 1, 0) called
[2]sys_pipe2: fd0=3, fd1=4, pipefd[3, 4], flags=0x0
dash call pipe: [3, 4]
[3]pipewrite: nread=0, nwrite=0, n=30, addr=pid 7, evaltree(0x42e658: 0, 1
[3]pipewrite: acq pi
[3]pipewrite: rel pi
[3]pipewrite: nread=0, nwrite=30, n=9, addr=) called

[3]pipewrite: acq pi
[3]pipewrite: rel pi
pid 8, evaltree(0x42e6f8: 0, 1) called
[1]pipewrite: nread=0, nwrite=39, n=94, addr=123
111
111
123
999
あいうえお
あいうえお
999
かきくけこ
12345
あいうえお

[1]pipewrite: acq pi
[1]pipewrite: rel pi
[3]piperead: nread=0, nwrite=133, n=1024
[3]piperead: acq pi
[3]piperead: rel pi
pid 7, evaltree(0x42e658: 0, 1) called
123
111
111
123
# sort test.txt | head -5
pid 6, evaltree(0x42e678: 1, 0) called
[1]sys_pipe2: fd0=3, fd1=4, pipefd[3, 4], flags=0x0
dash call pipe: [3, 4]
[3]pipewrite: nread=0, nwrite=0, n=30, addr=pid 9, evaltree(0x42e658: 0, 1
[3]pipewrite: acq pi
[3]pipewrite: rel pi
[3]pipewrite: nread=0, nwrite=30, n=9, addr=) called

[3]pipewrite: acq pi
[3]pipewrite: rel pi
pid 10, evaltree(0x42e6f8: 0, 1) called
[0]piperead: nread=0, nwrite=39, n=1024
[0]piperead: acq pi
[0]piperead: rel pi
pid 9, evaltree(0x42e658: 0, 1) called
[0]piperead: nread=39, nwrite=39, n=1024
[0]piperead: acq pi
QEMU: Terminated
```

- pipereadが先にlockを取得してdeadlockになっている模様

```
[2]sleep: release lk: 0xffff0000000cf1d0
# cat test.txt | head -5
[2]sleep: acquire lk: 0xffff0000000cf1d0
[3]sys_pipe2: fd0=3, fd1=4, pipefd[3, 4], flags=0x0
[2]sleep: release lk: 0xffff0000000a7128
[1]sleep: acquire lk: 0xffff0000000a7128
[1]sleep: release lk: 0xffff0000000a71c8
[1]sleep: acquire lk: 0xffff0000000a71c8
[3]pipewrite: [7]: nread=0, nwrite=0, n=94, addr='1'
[3]pipewrite: acq_lk
[3]pipewrite: rel_lk
[3]piperead: [8] nread=0, nwrite=94, n=1024
[3]piperead: acq_lk
[3]piperead: rel_lk
123
111
111
123
999
[3]sleep: release lk: 0xffff0000000cf1d0
# sort test.txt | head -5
[1]sleep: acquire lk: 0xffff0000000cf1d0
[1]sys_pipe2: fd0=3, fd1=4, pipefd[3, 4], flags=0x0
[1]piperead: [10] nread=0, nwrite=0, n=1024
[1]piperead: acq_lk
[1]piperead: to sleep with 0xffff00003aebd000
[1]sleep: release lk: 0xffff00003aebd000
QEMU: Terminated
```

### `cat test.txt | head -5 | wc -l`

- 7: cat < file (3), > p4(5)	: p3(4), (3)
- 8: head < p4(5), > p2(4)		: p1(3), p3(4)
- 9: wc < p1(3), > stdout		: p2(4)

- cat  {p3(4), p4(5)} head
- head {p1(3), p2(4)} wc

```
[0]execve: run [6] dash
[1]sys_close: [6] close 3
[3]sleep: release lk: 0xffff0000000cf1d0
# cat test.txt | head -5 | wc -l
[1]sleep: acquire lk: 0xffff0000000cf1d0
[2]sys_pipe2: pid 6: [3: 0xffff0000001fffc0, 4: 0xffff0000001fffe8] 	// p1(3), p2(4)
[1]yield: running proc 7
[2]sys_close: [6] close 4
[3]yield: running proc 7
[3]sys_close: [7] close 3					// file (3)
[3]sys_close: [7] close 4					// p3
[3]yield: running proc 7
[3]yield: running proc 7
[3]execve: parse [7] /usr/bin/cat
[2]sleep: release lk: 0xffff0000000a7308
[1]sleep: acquire lk: 0xffff0000000a7308
[1]sys_pipe2: pid 6: [4: 0xffff000000200010, 5: 0xffff000000200038]		// p3(4), p4(5)
[1]sys_close: [6] close 3
[1]sys_close: [6] close 5
[2]sys_close: [8] close 4					// p3
[2]sys_close: [8] close 3					// p1
[2]yield: running proc 8
[2]sys_close: [8] close 5					// p4
[0]yield: running proc 8
[0]execve: parse [8] /usr/bin/head
[1]sleep: release lk: 0xffff0000000a73a8
[2]sleep: acquire lk: 0xffff0000000a73a8
[2]sys_close: [6] close 4
[1]yield: running proc 9
[1]sys_close: [9] close 4					// p2
[1]yield: running proc 9
[1]execve: parse [9] /usr/bin/wc
[1]sleep: release lk: 0xffff0000000a6cb0
[1]sleep: acquire lk: 0xffff0000000a6cb0
[3]execve: run [7] cat
[3]yield: running proc 7
[3]sleep: release lk: 0xffff0000000a6cb0
[3]sleep: acquire lk: 0xffff0000000a6cb0
[0]execve: run [8] head
[3]yield: running proc 7
[2]yield: running proc 8
[2]yield: running proc 8
[3]yield: running proc 7
[2]fileread: [8] f=0xffff0000001fffc0			// p1
[2]piperead: [8] nread=0, nwrite=0, n=1024
[2]piperead: acq_lk
[2]piperead: to sleep with 0xffff00003af6f000
[3]filewrite: [7] f=0xffff0000001fffe8			// p2
[2]sleep: release lk: 0xffff00003af6f000
[3]pipewrite: [7]: nread=0, nwrite=0, n=94, addr='1'	// cat の出力
[3]pipewrite: acq_lk
[2]sleep: acquire lk: 0xffff00003af6f000
[3]pipewrite: rel_lk
[2]piperead: rel_lk
[3]yield: running proc 7
[2]yield: running proc 8
[3]sys_close: [7] close 3
[3]sys_close: [7] close 1
[3]yield: running proc 7
[3]sys_close: [7] close 2
[1]execve: run [9] wc
[1]yield: running proc 9
[1]yield: running proc 9
[1]fileread: [9] f=0xffff000000200010			// p3
[1]piperead: [9] nread=0, nwrite=0, n=16384
[1]piperead: acq_lk
[1]piperead: to sleep with 0xffff00003af04000
[1]sleep: release lk: 0xffff00003af04000
QEMU: Terminated
```

### rpi-os

```
# cat test.txt | head -5 | wc -l
[2]pipealloc: pi->nread; 0xffff00003af6f218, nwrite: 0xffff00003af6f21c
[2]sys_pipe2: pipefd[3, 4]
[2]pipealloc: pi->nread; 0xffff00003af04218, nwrite: 0xffff00003af0421c
[2]sys_pipe2: pipefd[4, 5]

[3]piperead: [8] nread=0, nwrite=0, n=1024				// headはpipe1[in]を読むがデータがないので
[3]piperead: [8] sleep: nread: 0xffff00003af6f218		// headはpipe1[in]待ちでsleep
[1]pipewrite: [7] nread=0, nwrite=0, n=94, addr='1'		// catがpipe1[out]に書き込む
[1]pipewrite: [7] pi->nwrite: 94
[1]pipewrite: [7] wakeup nread: 0xffff00003af6f218		// catはpipe1[in]待ちでsleepしているheadを起こす
[3]piperead: [8] pi->nread: 94							// headは起きて、pipe1[in]から読み込む
[3]piperead: [8] wakeup nwrite: 0xffff00003af6f21c		// headはpipe1[out]待ちを起こす（誰も待っていない）
														// headはpipe2[out]に書き込むはずだがしない
[2]piperead: [9] nread=0, nwrite=0, n=16384				// wcはpipe2[in]を読むがデータがないので
[2]piperead: [9] sleep: nread: 0xffff00003af04218		// wcはpipe2[in]待ちでsleep
QEMU: Terminated
```

```
# cat test.txt | head -5 | wc -l
[3]pipealloc: pi->nread; 0xffff00003af6f218, nwrite: 0xffff00003af6f21c	// pipe1
[3]sys_pipe2: pipefd[3, 4]
[3]sys_close: [6] fd=4, f: inum=-1
[1]sys_close: [7] fd=3, f: inum=-1									// catはpipe1[in]をclose
[1]sys_dup3: [7] fd1=4, fd2=1, flags=0, p->ofile[1]->ip->inum=-1	// catのoutはpipe1[out]
[1]sys_close: [7] fd=4, f: inum=-1

[1]pipealloc: pi->nread; 0xffff00003af04218, nwrite: 0xffff00003af0421c	// pipe2
[1]sys_pipe2: pipefd[4, 5]

[1]sys_close: [6] fd=3, f: inum=-1
[1]sys_close: [6] fd=5, f: inum=-1
[0]sys_close: [8] fd=4, f: inum=-1									// headはpipe1[out]をclose
[0]sys_dup3: [8] fd1=3, fd2=0, flags=0, p->ofile[0]->ip->inum=-1	// headのinはpipe1[in]
[0]sys_close: [8] fd=3, f: inum=-1
[0]sys_dup3: [8] fd1=5, fd2=1, flags=0, p->ofile[1]->ip->inum=-1	// headのoutはpipe2[out]
[0]sys_close: [8] fd=5, f: inum=-1
[1]sys_close: [6] fd=4, f: inum=-1
[3]sys_dup3: [9] fd1=4, fd2=0, flags=0, p->ofile[0]->ip->inum=-1	// wcのinはpipe2[in]
[3]sys_close: [9] fd=4, f: inum=-1
[2]sys_openat: [7] path=test.txt									// catのinはtest.txt
[0]piperead: [8] nread=0, nwrite=0, n=1024
[0]piperead: [8] sleep: nread: 0xffff00003af6f218					// headがreadでsleep
[3]pipewrite: [7]: nread=0, nwrite=0, n=94, addr='1'
[3]pipewrite: [7] pi->nwrite: 94									// catが書き込み
[3]pipewrite: [7] wakeup nread: 0xffff00003af6f218					// read待ちのheadをwakeup
[2]piperead: [8] pi->nread: 94										// headが読み込み
[2]piperead: [8] wakeup nwrite: 0xffff00003af6f21c
[3]sys_close: [7] fd=3, f: inum=31									// catはtest.txtをclose
[3]sys_close: [7] fd=1, f: inum=-1									// catはstdout(pipe1[out))をclose
[3]sys_close: [7] fd=2, f: inum=7									// catはstderr(/dev/tty)をclose
[1]piperead: [9] nread=0, nwrite=0, n=16384
[1]piperead: [9] sleep: nread: 0xffff00003af04218					// wcはread待ちでsleep
QEMU: Terminated
```

```
# cat test.txt | head -5 | wc -l
proc[6] sys_fstatat called
proc[6] sys_fstatat called
proc[6] sys_fstatat called
proc[6] sys_fstatat called
proc[6] sys_pipe2 called
pipealloc: pi->nread; 0xffff00003af6f218, nwrite: 0xffff00003af6f21c
sys_pipe2: pipefd[3, 4]
proc[6] sys_rt_sigprocmask called
proc[6] sys_rt_sigprocmask called
proc[6] sys_clone called
proc[7] sys_gettid called
proc[6] sys_rt_sigprocmask called
proc[7] sys_rt_sigprocmask called
proc[6] sys_rt_sigprocmask called
proc[7] sys_rt_sigprocmask called
proc[6] sys_setpgid called
proc[7] sys_getpid called
proc[6] sys_close called
proc[7] sys_setpgid called
sys_close: [6] fd=4, f: inum=-1
proc[7] sys_ioctl called
proc[6] sys_fstatat called
proc[7] sys_rt_sigaction called
proc[7] sys_rt_sigaction called
proc[7] sys_rt_sigaction called
proc[7] sys_rt_sigaction called
proc[7] sys_rt_sigaction called
proc[7] sys_close called
sys_close: [7] fd=3, f: inum=-1
proc[7] sys_dup3 called
sys_dup3: [7] fd1=4, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
proc[7] sys_close called
sys_close: [7] fd=4, f: inum=-1
proc[6] sys_fstatat called
proc[7] sys_mmap called
proc[7] sys_execve called
proc[6] sys_fstatat called
proc[6] sys_fstatat called
proc[6] sys_pipe2 called
pipealloc: pi->nread; 0xffff00003af04218, nwrite: 0xffff00003af0421c
sys_pipe2: pipefd[4, 5]
proc[6] sys_rt_sigprocmask called
proc[6] sys_rt_sigprocmask called
proc[6] sys_clone called
proc[8] sys_gettid called
proc[6] sys_rt_sigprocmask called
proc[8] sys_rt_sigprocmask called
proc[6] sys_rt_sigprocmask called
proc[8] sys_rt_sigprocmask called
proc[6] sys_setpgid called
proc[8] sys_setpgid called
proc[6] sys_close called
proc[8] sys_ioctl called
sys_close: [6] fd=3, f: inum=-1
proc[6] sys_close called
proc[8] sys_rt_sigaction called
sys_close: [6] fd=5, f: inum=-1
proc[8] sys_rt_sigaction called
proc[6] sys_fstatat called
proc[8] sys_rt_sigaction called
proc[8] sys_rt_sigaction called
proc[8] sys_rt_sigaction called
proc[8] sys_close called
sys_close: [8] fd=4, f: inum=-1
proc[8] sys_dup3 called
sys_dup3: [8] fd1=3, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
proc[6] sys_fstatat called
proc[8] sys_close called
sys_close: [8] fd=3, f: inum=-1
proc[8] sys_dup3 called
sys_dup3: [8] fd1=5, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
proc[8] sys_close called
sys_close: [8] fd=5, f: inum=-1
proc[6] sys_fstatat called
proc[8] sys_mmap called
proc[8] sys_execve called
proc[6] sys_fstatat called
proc[6] sys_rt_sigprocmask called
proc[6] sys_rt_sigprocmask called
proc[6] sys_clone called
proc[9] sys_gettid called
proc[6] sys_rt_sigprocmask called
proc[9] sys_rt_sigprocmask called
proc[6] sys_rt_sigprocmask called
proc[9] sys_rt_sigprocmask called
proc[6] sys_setpgid called
proc[9] sys_setpgid called
proc[6] sys_close called
proc[9] sys_ioctl called
sys_close: [6] fd=4, f: inum=-1
proc[9] sys_rt_sigaction called
proc[6] sys_close called
proc[9] sys_rt_sigaction called
proc[9] sys_rt_sigaction called
proc[6] sys_wait4 called
proc[9] sys_rt_sigaction called
proc[9] sys_rt_sigaction called
proc[9] sys_dup3 called
sys_dup3: [9] fd1=4, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
proc[9] sys_close called
sys_close: [9] fd=4, f: inum=-1
proc[9] sys_mmap called
proc[9] sys_execve called
proc[7] sys_gettid called
proc[7] sys_fstat called
proc[7] sys_openat called
sys_openat: [7] path=test.txt
proc[7] sys_fstat called
proc[7] sys_fadvise64 called
proc[8] sys_gettid called
proc[7] sys_mmap called
proc[8] sys_read called
proc[7] sys_brk called
piperead: [8] nread=0, nwrite=0, n=1024
piperead: [8] sleep: nread: 0xffff00003af6f218
proc[7] sys_brk called
proc[7] sys_mmap called
proc[7] sys_read called
proc[7] sys_write called
pipewrite: [7]: nread=0, nwrite=0, n=94, addr='1'
pipewrite: [7] pi->nwrite: 94
pipewrite: [7] wakeup nread: 0xffff00003af6f218
piperead: [8] pi->nread: 94
proc[7] sys_read called
piperead: [8] wakeup nwrite: 0xffff00003af6f21c
proc[7] sys_munmap called
proc[8] sys_lseek called
proc[8] sys_fstat called
proc[7] sys_close called
proc[8] sys_ioctl called
sys_close: [7] fd=3, f: inum=31
proc[7] sys_close called
sys_close: [7] fd=1, f: inum=-1
proc[7] sys_close called
sys_close: [7] fd=2, f: inum=7
proc[7] sys_exit called
proc[6] sys_wait4 called
proc[9] sys_gettid called
proc[9] sys_brk called
proc[9] sys_brk called
proc[9] sys_mmap called
proc[9] sys_mmap called
proc[9] sys_fadvise64 called
proc[9] sys_read called
piperead: [9] nread=0, nwrite=0, n=16384
piperead: [9] sleep: nread: 0xffff00003af04218
```

### xv6-fudanのdynamicブランチ

```
# cat test.txt | head -5 | wc -l
pipealloc: pi->nread; 0xffff00003922d218, nwrite: 0xffff00003922d21c
sys_pipe2: pipefd[3, 4]
pipealloc: pi->nread; 0xffff0000389b2218, nwrite: 0xffff0000389b221c
sys_pipe2: pipefd[4, 5]

pipewrite: [7] nread=0, nwrite=0, n=94, addr='1'		// catがpipe1[out]に書き込む
pipewrite: [7] pi->nwrite: 94
pipewrite: [7] wakeup nread: 0xffff00003922d218			// catはpipe1[in]待ちでsleepしているheadを起こす
piperead: [8] nread=0, nwrite=94, n=1024				// headはpipe1[in]から読み込む
piperead: [8] pi->nread: 94
piperead: [8] wakeup nwrite: 0xffff00003922d21c			// headはpipe2[out]待ちを起こす（誰も待っていない）
)
pipewrite: [8]: nread=0, nwrite=0, n=0, addr=''			// headはpiep2[out]に書き込もうとするが書き込むデータがない
pipewrite: [8] pi->nwrite: 0
pipewrite: [8] wakeup nread: 0xffff0000389b2218			// headはpipe2[in]待ちを起こす（まだ誰も待っていない）
pipewrite: [8] nread=0, nwrite=0, n=20, addr='1'		// headはpipe2[out]に書き込む
pipewrite: [8] pi->nwrite: 20
pipewrite: [8] wakeup nread: 0xffff0000389b2218			// headはpipe2[in]待ちを起こす
piperead: [9] nread=0, nwrite=20, n=16384				// wcはpipe2[in]から読み込む
piperead: [9] pi->nread: 20
piperead: [9] wakeup nwrite: 0xffff0000389b221c			// wcはpipe2[out]待ちを起こす（誰も待っていな）
piperead: [9] nread=20, nwrite=20, n=16384
piperead: [9] pi->nread: 20
piperead: [9] wakeup nwrite: 0xffff0000389b221c
```

```
# sort test.txt | head -5
pipealloc: pi->nread; 0xffff000039aa7218, nwrite: 0xffff000039aa721c
sys_pipe2: pipefd[3, 4]

piperead: [8] nread=0, nwrite=0, n=1024
piperead: [8] sleep: nread: 0xffff000039aa7218
pipewrite: [7] nread=0, nwrite=0, n=0, addr=''
pipewrite: [7] pi->nwrite: 0
pipewrite: [7] wakeup nread: 0xffff000039aa7218
pipewrite: [7] nread=0, nwrite=0, n=4, addr='1'
pipewrite: [7] pi->nwrite: 4
pipewrite: [7] wakeup nread: 0xffff000039aa7218
piperead: [8] pi->nread: 4
piperead: [8] wakeup nwrite: 0xffff000039aa721c
111
piperead: [8] nread=4, nwrite=4, n=1024
piperead: [8] sleep: nread: 0xffff000039aa7218
pipewrite: [7] nread=4, nwrite=4, n=90, addr='1'
pipewrite: [7] pi->nwrite: 94
pipewrite: [7] wakeup nread: 0xffff000039aa7218
pipewrite: [7] nread=4, nwrite=94, n=0, addr=''
pipewrite: [7] pi->nwrite: 94
pipewrite: [7] wakeup nread: 0xffff000039aa7218
piperead: [8] pi->nread: 94
piperead: [8] wakeup nwrite: 0xffff000039aa721c
111
123
123
12345
#
```

```
# cat test.txt | head -5 | wc -l
pipealloc: pi->nread; 0xffff00003afb3218, nwrite: 0xffff00003afb321c
sys_pipe2: pipefd[3, 4]
[7] fd=3, f->inum=-1
[7] fd1=4, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
[7] fd=4, f->inum=-1
[6] fd=4, f->inum=-1
pipealloc: pi->nread; 0xffff00003a32d218, nwrite: 0xffff00003a32d21c
sys_pipe2: pipefd[4, 5]
[6] fd=3, f->inum=-1
[6] fd=5, f->inum=-1
[9] fd1=4, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
[9] fd=4, f->inum=-1
[6] fd=4, f->inum=-1
[8] fd=4, f->inum=-1
[8] fd1=3, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
[8] fd=3, f->inum=-1
[8] fd1=5, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
[8] fd=5, f->inum=-1
pipewrite: [7] nread=0, nwrite=0, n=94, addr='1'
pipewrite: [7] pi->nwrite: 94
pipewrite: [7] wakeup nread: 0xffff00003afb3218
[7] fd=3, f->inum=159
[7] fd=1, f->inum=-1
[7] fd=2, f->inum=7
piperead: [8] nread=0, nwrite=94, n=1024
piperead: [8] pi->nread: 94
piperead: [8] wakeup nwrite: 0xffff00003afb321c
filelseek: invalid offset -74
pipewrite: [8] nread=0, nwrite=0, n=0, addr=''
pipewrite: [8] pi->nwrite: 0
pipewrite: [8] wakeup nread: 0xffff00003a32d218
pipewrite: [8] nread=0, nwrite=0, n=20, addr='1'
pipewrite: [8] pi->nwrite: 20
pipewrite: [8] wakeup nread: 0xffff00003a32d218
piperead: [9] nread=0, nwrite=20, n=16384
piperead: [9] pi->nread: 20
piperead: [9] wakeup nwrite: 0xffff00003a32d21c
piperead: [9] nread=20, nwrite=20, n=16384
piperead: [9] sleep: nread: 0xffff00003a32d218
[8] fd=0, f->inum=-1
[8] fd=1, f->inum=-1
[8] fd=2, f->inum=7
piperead: [9] pi->nread: 20
piperead: [9] wakeup nwrite: 0xffff00003a32d21c
5
[9] fd=0, f->inum=-1
[9] fd=1, f->inum=7
[9] fd=2, f->inum=7
```

```
cat test.txt | head -5 | wc -l
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_pipe2
pipealloc: pi->nread; 0xffff00003afb3218, nwrite: 0xffff00003afb321c
sys_pipe2: pipefd[3, 4]
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_clone
proc[7] calls sys_gettid
proc[7] calls sys_rt_sigprocmask
proc[7] calls sys_rt_sigprocmask
proc[7] calls sys_getpid
proc[7] calls sys_setpgid
proc[7] calls sys_ioctl
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_setpgid
proc[6] calls sys_close
[6] fd=4, f->inum=-1
proc[6] calls sys_fstatat
proc[7] calls sys_rt_sigaction
proc[7] calls sys_rt_sigaction
proc[7] calls sys_rt_sigaction
proc[7] calls sys_rt_sigaction
proc[7] calls sys_rt_sigaction
proc[7] calls sys_close
[7] fd=3, f->inum=-1
proc[7] calls sys_dup3
[7] fd1=4, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_pipe2
pipealloc: pi->nread; 0xffff00003a32d218, nwrite: 0xffff00003a32d21c
sys_pipe2: pipefd[4, 5]
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_clone
proc[7] calls sys_close
[7] fd=4, f->inum=-1
proc[8] calls sys_gettid
proc[8] calls sys_rt_sigprocmask
proc[8] calls sys_rt_sigprocmask
proc[8] calls sys_setpgid
proc[8] calls sys_ioctl
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_setpgid
proc[6] calls sys_close
[6] fd=3, f->inum=-1
proc[6] calls sys_close
[6] fd=5, f->inum=-1
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[6] calls sys_fstatat
proc[7] calls sys_execve
proc[8] calls sys_rt_sigaction
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_clone
proc[9] calls sys_gettid
proc[9] calls sys_rt_sigprocmask
proc[9] calls sys_rt_sigprocmask
proc[9] calls sys_setpgid
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_rt_sigprocmask
proc[6] calls sys_setpgid
proc[6] calls sys_close
[6] fd=4, f->inum=-1
proc[6] calls sys_close
proc[6] calls sys_wait4
proc[9] calls sys_ioctl
proc[9] calls sys_rt_sigaction
proc[9] calls sys_rt_sigaction
proc[9] calls sys_rt_sigaction
proc[9] calls sys_rt_sigaction
proc[8] calls sys_rt_sigaction
proc[8] calls sys_rt_sigaction
proc[8] calls sys_rt_sigaction
proc[9] calls sys_rt_sigaction
proc[9] calls sys_dup3
[9] fd1=4, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
proc[9] calls sys_close
[9] fd=4, f->inum=-1
proc[8] calls sys_rt_sigaction
proc[8] calls sys_close
[8] fd=4, f->inum=-1
proc[8] calls sys_dup3
[8] fd1=3, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
proc[9] calls sys_execve
proc[8] calls sys_close
[8] fd=3, f->inum=-1
proc[8] calls sys_dup3
[8] fd1=5, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
proc[8] calls sys_close
[8] fd=5, f->inum=-1
proc[8] calls sys_execve
proc[7] calls sys_gettid
proc[7] calls sys_fstat
proc[7] calls sys_openat
proc[7] calls sys_fstat
proc[7] calls sys_fadvise64
proc[7] calls sys_brk
proc[7] calls sys_brk
proc[7] calls sys_read
proc[7] calls sys_write
pipewrite: [7] nread=0, nwrite=0, n=95, addr='1'
pipewrite: [7] pi->nwrite: 95
pipewrite: [7] wakeup nread: 0xffff00003afb3218
proc[7] calls sys_read
proc[7] calls sys_close
[7] fd=3, f->inum=159
proc[7] calls sys_close
[7] fd=1, f->inum=-1
proc[7] calls sys_close
[7] fd=2, f->inum=7
proc[7] calls sys_exit
proc[6] calls sys_wait4
proc[9] calls sys_gettid
proc[8] calls sys_gettid
proc[9] calls sys_brk
proc[9] calls sys_brk
proc[8] calls sys_read
piperead: [8] nread=0, nwrite=95, n=1024
piperead: [8] pi->nread: 95
piperead: [8] wakeup nwrite: 0xffff00003afb321c
proc[8] calls sys_lseek
filelseek: invalid offset -75
proc[9] calls sys_fadvise64
proc[9] calls sys_read
piperead: [9] nread=0, nwrite=0, n=16384
piperead: [9] sleep: nread: 0xffff00003a32d218
proc[8] calls sys_fstat
proc[8] calls sys_ioctl
proc[8] calls sys_writev
pipewrite: [8] nread=0, nwrite=0, n=0, addr=''
pipewrite: [8] pi->nwrite: 0
pipewrite: [8] wakeup nread: 0xffff00003a32d218
pipewrite: [8] nread=0, nwrite=0, n=20, addr='1'
pipewrite: [8] pi->nwrite: 20
pipewrite: [8] wakeup nread: 0xffff00003a32d218
piperead: [9] pi->nread: 20
piperead: [9] wakeup nwrite: 0xffff00003a32d21c
proc[9] calls sys_read
piperead: [9] nread=20, nwrite=20, n=16384
piperead: [9] sleep: nread: 0xffff00003a32d218
proc[8] calls sys_close
[8] fd=0, f->inum=-1
proc[8] calls sys_close
[8] fd=1, f->inum=-1
proc[8] calls sys_close
[8] fd=2, f->inum=7
proc[8] calls sys_exit
piperead: [9] pi->nread: 20
piperead: [9] wakeup nwrite: 0xffff00003a32d21c
proc[6] calls sys_wait4
proc[9] calls sys_writev
5
proc[9] calls sys_close
[9] fd=0, f->inum=-1
proc[9] calls sys_close
[9] fd=1, f->inum=7
proc[9] calls sys_close
[9] fd=2, f->inum=7
proc[9] calls sys_exit
proc[6] calls sys_wait4
proc[6] calls sys_ioctl
proc[6] calls sys_write
# proc[6] calls sys_read
```

## pipe, dupのテストプログラムを用意

- [https://www.haya-programming.com/entry/2018/11/08/185349](https://www.haya-programming.com/entry/2018/11/08/185349)を一部変更して使用

### Macで実行

```
$ ./pipe
dopipes(0)
pipe: pp=[3, 4]
[p] close pp[1] = 4
[p] dup pp[0] = 3 to 0
[p] close pp[0] = 3
[0] execvp: wc
[c] close pp[0] = 3
[c] dup pp[1] = 4 to 1
[c] close pp[1] = 4
dopipes(1)
pipe: pp=[3, 4]
[p] close pp[1] = 4
[p] dup pp[0] = 3 to 0
[p] close pp[0] = 3
[1] execvp: head
[c] close pp[0] = 3
[c] dup pp[1] = 4 to 1
[c] close pp[1] = 4
dopipes(2)
[2] execvp cat
       5
```

### xv6で実行

```
# pipetest
dopipes(0)
pipe: pp=[3, 4]
[p] close pp[1] = 4
[p] dup pp[0] = 3 to 0
[p] close pp[0] = 3
[c] close pp[0] = 3
[0] execvp: wc
[c] dup pp[1] = 4 to 1
[c] close pp[1] = 4
dopipes(1)
pipe: pp=[3, 4]
[p] close pp[1] = 4
[p] dup pp[0] = 3 to 0
[p] close pp[0] = 3
[1] execvp: head
[c] close pp[0] = 3
[c] dup pp[1] = 4 to 1
[c] close pp[1] = 4
dopipes(2)						// ここでストール
QEMU: Terminated
```

## sys_ioctlのバグだった

- fがpipeの場合、f->ipはないためf->ip->typeがエラー

```diff
@@ -192,9 +193,9 @@ sys_ioctl()
     if (argfd(0, &fd, &f) < 0 || argu64(1, &req) < 0)
         return -EINVAL;

-    trace("fd: %d, req: 0x%llx, f->type: %d", fd, req, f->ip->type);
+    trace("fd=%d, req=0x%llx, type=%d", fd, req, f->type);

-    if (f->ip->type != T_DEV) return -ENOTTY;
+    if (f->type != FD_INODE || f->ip->type != T_DEV) return -ENOTTY;

     trace("fd=%d, req=0x%llx\n", fd, req);
```

```
# cat test.txt | head -5 | wc -l
[2]pipealloc: pi->nread; 0xffff00003af6f218, nwrite: 0xffff00003af6f21c
[2]sys_pipe2: pipefd[3, 4]
[0]sys_ioctl: fd=10, req=0x5410, type=2
[2]sys_close: [6] fd=4, f: inum=-1
[0]sys_close: [7] fd=3, f: inum=-1
[0]sys_dup3: [7] fd1=4, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
[1]sys_close: [7] fd=4, f: inum=-1
[2]pipealloc: pi->nread; 0xffff00003af04218, nwrite: 0xffff00003af0421c
[2]sys_pipe2: pipefd[4, 5]
[0]sys_close: [6] fd=3, f: inum=-1
[0]sys_close: [6] fd=5, f: inum=-1
[3]sys_ioctl: fd=10, req=0x5410, type=2
[3]sys_close: [8] fd=4, f: inum=-1
[3]sys_dup3: [8] fd1=3, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
[3]sys_close: [8] fd=3, f: inum=-1
[3]sys_dup3: [8] fd1=5, fd2=1, flags=0, p->ofile[1]->ip->inum=-1
[3]sys_close: [8] fd=5, f: inum=-1
[0]sys_close: [6] fd=4, f: inum=-1
[2]sys_ioctl: fd=10, req=0x5410, type=2
[2]sys_dup3: [9] fd1=4, fd2=0, flags=0, p->ofile[0]->ip->inum=-1
[2]sys_close: [9] fd=4, f: inum=-1
[1]sys_openat: [7] path=test.txt
[3]piperead: [8] nread=0, nwrite=0, n=1024
[3]piperead: [8] sleep: nread: 0xffff00003af6f218
[1]pipewrite: [7]: nread=0, nwrite=0, n=94, addr='1'
[1]pipewrite: [7] pi->nwrite: 94
[1]pipewrite: [7] wakeup nread: 0xffff00003af6f218
[3]piperead: [8] pi->nread: 94
[3]piperead: [8] wakeup nwrite: 0xffff00003af6f21c
[3]sys_lseek: [8] fd=0, f->inum=-1, offset=-74, whence=1
[1]sys_close: [7] fd=3, f: inum=31
[3]filelseek: f->off: 0, +offset: -74
[1]sys_close: [7] fd=1, f: inum=-1
[3]filelseek: invalid offset -74
[1]sys_close: [7] fd=2, f: inum=7
[3]sys_ioctl: fd=1, req=0x5413, type=1
[3]pipewrite: [8]: nread=0, nwrite=0, n=0, addr=''
[3]pipewrite: [8] pi->nwrite: 0
[3]pipewrite: [8] wakeup nread: 0xffff00003af04218
[3]pipewrite: [8]: nread=0, nwrite=0, n=20, addr='1'
[3]pipewrite: [8] pi->nwrite: 20
[3]pipewrite: [8] wakeup nread: 0xffff00003af04218
[3]sys_close: [8] fd=0, f: inum=-1
[3]sys_close: [8] fd=1, f: inum=-1
[3]sys_close: [8] fd=2, f: inum=7
[1]piperead: [9] nread=0, nwrite=20, n=16384
[1]piperead: [9] pi->nread: 20
[1]piperead: [9] wakeup nwrite: 0xffff00003af0421c
[1]piperead: [9] nread=20, nwrite=20, n=16384
[1]piperead: [9] pi->nread: 20
[1]piperead: [9] wakeup nwrite: 0xffff00003af0421c
5
[2]sys_close: [9] fd=0, f: inum=-1
[2]sys_close: [9] fd=1, f: inum=7
[2]sys_close: [9] fd=2, f: inum=7
[1]sys_ioctl: fd=10, req=0x5410, type=2
```

```
# cat test.txt | head -5 | wc -l
5
# 1 sleep  init
2 runble idle
3 run    idle
4 run    idle
5 run    idle
6 sleep  dash fa: 1
# cat test.txt | head -5 | cat
123
111
111
123
999
```

## `sort test.txt | wc -5`は依然としてストール

- ただし、別のコマンドでエラー発生
- デバッグプリントを入れると`sort test.txt | wc -5`も動く

```
# sort test.txt | wc -l
[1]sys_lseek: [7] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [7] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [7] fd=3, f.type=2, offset=0, whence=1
[2]sys_lseek: [7] fd=3, f.type=2, offset=94, whence=0
sort: fflush failed: 'standard output': Bad address
sort: write error
[1]exit: exit: pid 7, name sort, err 2
11
[3]sys_write: [6] fd: 2, p: # , n: 2, f->type: 2
# sort test.txt | head -5
[0]sys_lseek: [9] fd=3, f.type=2, offset=0, whence=1
[3]sys_lseek: [9] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [9] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [9] fd=3, f.type=2, offset=94, whence=0
111
[0]sys_lseek: [10] fd=0, f.type=1, offset=-72, whence=1
111
123
123
12345
sort: fflush failed: 'standard output': Bad address     // sys_writevでエラー
sort: write error
[1]exit: exit: pid 9, name sort, err 2
[2]sys_write: [6] fd: 2, p: # , n: 2, f->type: 2
#
```

```
# sort test.txt | wc -l
[0]sys_lseek: [7] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [7] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [7] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [7] fd=3, f.type=2, offset=94, whence=0   // 94 = test.txtのサイズ
[3]sys_writev: [7] fd 1, iovcnt: 2
len=0: base[0]: '*'                                     // 以降の2つのsys_writevはsort.txtの出力
len=4: base[0]: '1'                                     // 111\n
[3]sys_writev: [7] fd 1, iovcnt: 2
len=90: base[0]: '1'                                    // test.txtの111\n以降
[3]sys_writev: [7] fd 2, iovcnt: 2                      // 以降のsys_writevはsortのエラー出力
len=6: base[0]: 's'
sort:
[3]sys_writev: [7] fd 2, iovcnt: 2
len=0: base[0]: '*'                                     // このiovでEFAULT発生
len=32: base[0]: 'f'
fflush failed: 'standard output'
[2]sys_writev: [7] fd 2, iovcnt: 2
len=13: base[0]: ':'
: Bad address[2]sys_writev: [7] fd 2, iovcnt: 2
len=0: base[0]: '*'
len=1: base[0]: '$'

[2]sys_writev: [7] fd 2, iovcnt: 2
len=6: base[0]: 's'
sort:
[2]sys_writev: [7] fd 2, iovcnt: 2
len=0: base[0]: '*'
len=11: base[0]: 'w'
write error
[0]sys_writev: [8] fd 1, iovcnt: 2
len=2:
[2]sys_wribase[0t]ev: ': 1'
[17] f1d 2, iovlcen=1nt: :2 b
ase[0]: '$'
len=0: base[0]: '*'
len=1: base[0]: '$'


[3]exit: exit: pid 7, name sort, err 2
[0]sys_write: [6] fd: 2, p: # , n: 2, f->type: 2
# sort test.txt | head -5
[1]sys_lseek: [9] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [9] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [9] fd=3, f.type=2, offset=0, whence=1
[1]sys_lseek: [9] fd=3, f.type=2, offset=94, whence=0
[0]sys_writev: [9] fd 1, iovcnt: 2
len=0: base[0]: '*'
len=4: base[0]: '1'
[1]sys_writev: [10] fd 1, iovcnt: 2
len=0: base[0]: '*'
len=4: base[0]: '1'
[0]sys_writev: [9] fd 1, iovcnt: 2
len=90: base[0]: '1'
111
[1]sys_lseek: [10] fd=0, f.type=1, offset=-72, whence=1
[1]sys_writev: [10] fd 1, iovcnt: 2
len=0: base[0]: '*'
len=18: base[0]: '1'
[0]sys_writev: [9] fd 2, iovcnt: 2
len=6: base[0]: 's'
111
123
123
12345
sort: [3]sys_writev: [9] fd 2, iovcnt: 2
len=0: base[0]: '*'
len=32: base[0]: 'f'
fflush failed: 'standard output'[3]sys_writev: [9] fd 2, iovcnt: 2
len=13: base[0]: ':'
: Bad address[3]sys_writev: [9] fd 2, iovcnt: 2
len=0: base[0]: '*'
len=1: base[0]: '$'

[3]sys_writev: [9] fd 2, iovcnt: 2
len=6: base[0]: 's'
sort: [3]sys_writev: [9] fd 2, iovcnt: 2
len=0: base[0]: '*'
len=11: base[0]: 'w'
write error[3]sys_writev: [9] fd 2, iovcnt: 2
len=0: base[0]: '*'
len=1: base[0]: '$'

[3]exit: exit: pid 9, name sort, err 2
[3]sys_write: [6] fd: 2, p: # , n: 2, f->type: 2
```

## fflush時に実行されるf->write(f, 0, 0)をエラーにしていた

- sys_writevでiov_base=0となりin_user()でエラーとなり-EFAULTを返していた
- 上記の場合は読み飛ばすようにした

```
# sort test.txt | wc -l
11
# sort test.txt | head -5
111
111
123
123
12345
# sort test.txt | uniq -c
      2 111
      2 123
      1 12345
      2 999
      3 あいうえお
      1 かきくけこ
# sort test.txt | uniq -c | head -3
[3]exit: exit: pid 15, name head, err 1
# sort test.txt | head -10 | uniq -c
      2 111
      2 123
      1 12345
      2 999
      3 あいうえお
# sort test.txt | uniq -c | head -n 3
[2]exit: exit: pid 21, name head, err 1
```

## headがexit(1)で終了している

- エラーメッセージはproc()#exit()で出力
- デバッグ出力したら出力された

```
# [0]syscall1: proc[6] sys_read called
sort test.txt | uniq -c | head -3
[2]syscall1: proc[6] sys_fstatat called
[3]syscall1: proc[6] sys_fstatat called
[2]syscall1: proc[6] sys_fstatat called
[1]syscall1: proc[6] sys_fstatat called
[0]syscall1: proc[6] sys_pipe2 called
[2]syscall1: proc[6] sys_rt_sigprocmask called
[2]syscall1: proc[6] sys_rt_sigprocmask called
[2]syscall1: proc[6] sys_clone called
[1]syscall1: proc[7] sys_gettid called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[1]syscall1: proc[7] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[1]syscall1: proc[7] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_setpgid called
[1]syscall1: proc[7] sys_getpid called
[1]syscall1: proc[7] sys_setpgid called
[0]syscall1: proc[6] sys_close called
[1]syscall1: proc[7] sys_ioctl called
[0]syscall1: proc[6] sys_fstatat called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_close called
[2]syscall1: proc[7] sys_dup3 called
[0]syscall1: proc[6] sys_fstatat called
[2]syscall1: proc[7] sys_close called
[3]syscall1: proc[7] sys_mmap called
[3]syscall1: proc[7] sys_execve called
[0]syscall1: proc[6] sys_fstatat called
[0]syscall1: proc[6] sys_fstatat called
[0]syscall1: proc[6] sys_pipe2 called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_clone called
[2]syscall1: proc[8] sys_gettid called
[2]syscall1: proc[8] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[2]syscall1: proc[8] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[1]syscall1: proc[8] sys_setpgid called
[0]syscall1: proc[6] sys_setpgid called
[1]syscall1: proc[8] sys_ioctl called
[0]syscall1: proc[6] sys_close called
[1]syscall1: proc[8] sys_rt_sigaction called
[0]syscall1: proc[6] sys_close called
[1]syscall1: proc[8] sys_rt_sigaction called
[0]syscall1: proc[6] sys_fstatat called
[1]syscall1: proc[8] sys_rt_sigaction called
[1]syscall1: proc[8] sys_rt_sigaction called
[1]syscall1: proc[8] sys_rt_sigaction called
[1]syscall1: proc[8] sys_close called
[1]syscall1: proc[8] sys_dup3 called
[1]syscall1: proc[8] sys_close called
[1]syscall1: proc[8] sys_dup3 called
[1]syscall1: proc[8] sys_close called
[1]syscall1: proc[8] sys_mmap called
[0]syscall1: proc[6] sys_fstatat called
[1]syscall1: proc[8] sys_execve called
[0]syscall1: proc[6] sys_fstatat called
[0]syscall1: proc[6] sys_fstatat called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_rt_sigprocmask called
[0]syscall1: proc[6] sys_clone called
[0]syscall1: proc[9] sys_gettid called
[2]syscall1: proc[6] sys_rt_sigprocmask called
[0]syscall1: proc[9] sys_rt_sigprocmask called
[2]syscall1: proc[6] sys_rt_sigprocmask called
[0]syscall1: proc[9] sys_rt_sigprocmask called
[2]syscall1: proc[6] sys_setpgid called
[2]syscall1: proc[6] sys_close called
[2]syscall1: proc[6] sys_close called
[0]syscall1: proc[9] sys_setpgid called
[2]syscall1: proc[6] sys_wait4 called
[0]syscall1: proc[9] sys_ioctl called
[0]syscall1: proc[9] sys_rt_sigaction called
[0]syscall1: proc[9] sys_rt_sigaction called
[0]syscall1: proc[9] sys_rt_sigaction called
[0]syscall1: proc[9] sys_rt_sigaction called
[0]syscall1: proc[9] sys_rt_sigaction called
[0]syscall1: proc[9] sys_dup3 called
[0]syscall1: proc[9] sys_close called
[0]syscall1: proc[9] sys_mmap called
[0]syscall1: proc[9] sys_execve called
[1]syscall1: proc[8] sys_gettid called
[1]syscall1: proc[8] sys_fadvise64 called
[1]syscall1: proc[8] sys_read called
[1]syscall1: proc[7] sys_gettid called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[1]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigprocmask called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_rt_sigaction called
[3]syscall1: proc[7] sys_brk called
[3]syscall1: proc[7] sys_brk called
[3]syscall1: proc[7] sys_mmap called
[3]syscall1: proc[7] sys_mmap called
[3]syscall1: proc[7] sys_faccessat2 called
[3]syscall1: proc[7] sys_sched_getaffinity called
[3]syscall1: proc[7] sys_sched_getaffinity called
[3]syscall1: proc[7] sys_openat called
[3]syscall1: proc[7] sys_fcntl called
[3]syscall1: proc[7] sys_mmap called
[3]syscall1: proc[7] sys_fadvise64 called
[3]syscall1: proc[7] sys_fstat called
[0]syscall1: proc[7] sys_prlimit64 called
[0]syscall1: proc[7] sys_prlimit64 called
[0]syscall1: proc[7] sys_prlimit64 called
[0]syscall1: proc[7] sys_sysinfo called
[0]syscall1: proc[7] sys_sysinfo called
[0]syscall1: proc[7] sys_mmap called
[0]syscall1: proc[7] sys_readv called
[0]syscall1: proc[7] sys_read called
[0]syscall1: proc[7] sys_lseek called
[0]syscall1: proc[7] sys_lseek called
[0]syscall1: proc[7] sys_lseek called
[0]syscall1: proc[7] sys_lseek called
[0]syscall1: proc[7] sys_close called
[0]syscall1: proc[7] sys_munmap called
[0]syscall1: proc[7] sys_ioctl called
[3]syscall1: proc[7] sys_writev called
[0]syscall1: proc[8] sys_brk called
[3]syscall1: proc[7] sys_writev called
[0]syscall1: proc[8] sys_brk called
[3]syscall1: proc[7] sys_munmap called
[0]syscall1: proc[8] sys_mmap called
[3]syscall1: proc[7] sys_close called
[3]syscall1: proc[7] sys_close called
[3]syscall1: proc[7] sys_exit called
[0]syscall1: proc[8] sys_mmap called
[2]syscall1: proc[9] sys_gettid called
[3]syscall1: proc[6] sys_wait4 called
[0]syscall1: proc[8] sys_read called
[0]syscall1: proc[8] sys_ioctl called
[0]syscall1: proc[8] sys_writev called
[1]syscall1: proc[9] sys_read called
[0]syscall1: proc[8] sys_read called
[1]syscall1: proc[9] sys_ioctl called
[1]syscall1: proc[9] sys_writev called
[0] s     2 ys111c
ll1: proc[8] sys_lseek called
[1]syscall1: proc[9] sys_read called
[0]syscall1: proc[8] sys_lseek called
[0]syscall1: proc[8] sys_lseek called
[0]syscall1: proc[8] sys_lseek called
[0]syscall1: proc[8] sys_close called
[0]syscall1: proc[8] sys_munmap called
[0]syscall1: proc[8] sys_writev called
[0]syscall1: proc[8] sys_close called
[2]syscall1: proc[9] sys_lseek called
[0]syscall1: proc[8] sys_close called
[2]syscall1: proc[9] sys_fstat called
[0]syscall1: proc[8] sys_exit called
[2]syscall1: proc[9] sys_writev called
      2 123
      1 12345
[0]syscall1: proc[6] sys_wait4 called
[2]syscall1: proc[9] sys_close called
[2]syscall1: proc[9] sys_close called
[2]syscall1: proc[9] sys_close called
[0]syscall1: proc[9] sys_exit called
[0]syscall1: proc[6] sys_wait4 called
[0]syscall1: proc[6] sys_ioctl called
[0]syscall1: proc[6] sys_mmap called
[0]syscall1: proc[6] sys_write called
# [0]syscall1: proc[6] sys_munmap called
```

- コマンドの実行順で結果が変わる

```
# sort test.txt | uniq -c | head -3
      2 111
      2 123
      1 12345
# sort test.txt | head -10 | head -3
111
111
123
# sort test.txt | head -10 | uniq -c
      2 111
      2 123
      1 12345
      2 999
      3 あいうえお
# sort test.txt | uniq -c | head -3
[1]exit: exit: pid 18, name head, err 1
#
```

### １回目の実行はOK

```
# sort test.txt | uniq -c | head -3
[1]syscall1: proc[9] sys_gettid called
[1]syscall1: proc[9] sys_rt_sigprocmask called
[1]syscall1: proc[9] sys_rt_sigprocmask called
[1]syscall1: proc[9] sys_setpgid called
[1]syscall1: proc[9] sys_ioctl called
[1]syscall1: proc[9] sys_rt_sigaction called
[1]syscall1: proc[9] sys_rt_sigaction called
[1]syscall1: proc[9] sys_rt_sigaction called
[1]syscall1: proc[9] sys_rt_sigaction called
[1]syscall1: proc[9] sys_rt_sigaction called
[1]syscall1: proc[9] sys_dup3 called
[1]syscall1: proc[9] sys_close called
[1]syscall1: proc[9] sys_mmap called
[1]syscall1: proc[9] sys_execve called
[1]syscall1: proc[9] sys_gettid called
[1]syscall1: proc[9] sys_read called
[1]syscall1: proc[9] sys_lseek called
[1]syscall1: proc[9] sys_fstat called
[1]syscall1: proc[9] sys_ioctl called
[1]syscall1: proc[9] sys_writev called
      2 111
      2 123
      1 12345
[1]syscall1: proc[9] sys_close called
[1]syscall1: proc[9] sys_close called
[1]syscall1: proc[9] sys_close called
[1]syscall1: proc[9] sys_exit called
# QEMU: Terminated
```

### 2回目の実行はNG

```
# sort test.txt | uniq -c | head -3
      2 111
      2 123
      1 12345
# sort test.txt | uniq -c
      2 111
      2 123
      1 12345
      2 999
      3 あいうえお
      1 かきくけこ
# sort test.txt | uniq -c | head -3
[1]syscall1: proc[14] sys_gettid called
[1]syscall1: proc[14] sys_rt_sigprocmask called
[1]syscall1: proc[14] sys_rt_sigprocmask called
[1]syscall1: proc[14] sys_setpgid called
[1]syscall1: proc[14] sys_ioctl called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_dup3 called
[1]syscall1: proc[14] sys_close called
[1]syscall1: proc[14] sys_mmap called
[1]syscall1: proc[14] sys_execve called
[3]syscall1: proc[14] sys_gettid called
[3]syscall1: proc[14] sys_read called
[0]exit: exit: head [14]: err 1
# QEMU: Terminated
```

### 未対応trapが発生していた

- farはheaのtext内のアドレス
- headは途中まで処理が進んでいる
- 今更何故このアドレスのtranslationが失敗するのかわからない

```
[0]trap: unknown trap code: 32
[0]exit: [14] head exit with 1

iabort: esr=0x82000004, ec=32, dfs=4, far=0x41a4bc             // 1回発生
iabort: esr=0x82000007, ec=32, dfs=7, far=0x41a4bc             // 直後からendlessに発生
            0b100000_1_000000000000_00_0_0_0_0_0_000100        // SET[12:11]: Recoverable state (if FEAT_RAS)
                                                               // FnV[10]: FAR is valid
                                                               // S1PTW:[7] Fault not on a stage 2
                                                               // IFSC[5:0]: 4: Translation fault, level 0
                                                               // IFSC[5:0]: 7: Translation fault, level 3
```

```
000000000041a49c <__syscall_cp_c>:
  41a49c:   aa0003e8    mov x8, x0
  41a4a0:   aa0103e0    mov x0, x1
  41a4a4:   aa0203e1    mov x1, x2
  41a4a8:   aa0303e2    mov x2, x3
  41a4ac:   aa0403e3    mov x3, x4
  41a4b0:   aa0503e4    mov x4, x5
  41a4b4:   aa0603e5    mov x5, x6
  41a4b8:   d4000001    svc #0x0
  41a4bc:   d65f03c0    ret
```

### trap.c

```c
    if (dfs <= 7) {             // Translation fault
        lttbr0((uint64_t)p->pgdir);                                   // そのプロセスのlevel 0を設定
        if ((pte = pgdir_walk(p->pgdir, (void *)far, 1)) == 0) {      // その他のレベルを作成
            warn("[%d] recoveary failed: dfs=%d, far=0x%llx", p->pid, dfs, far);
            return -1;
        } else {
            info("pte=0x%llx, *pte=0x%llx", pte, *pte);
            return 0;
        }
```

```
[2]trap: iabort: esr=0x82000004, ec=32, dfs=4, far=0x41a4bc    //
[2]pf_handler: pte=0xffff00003acda0d0, *pte=0x0                // paがない
[2]trap: iabort: esr=0x82000007, ec=32, dfs=7, far=0x41a4bc
[2]pf_handler: pte=0xffff00003acda0d0, *pte=0x0                // paがない
```

```
# [0]sys_read: [6] f->type=2, p=0x42dfd0, n=1024
sort test.txt | uniq -c | head -3
[1]syscall1: proc[14] sys_gettid called
[1]syscall1: proc[14] sys_rt_sigprocmask called
[1]syscall1: proc[14] sys_rt_sigprocmask called
[1]syscall1: proc[14] sys_setpgid called
[1]syscall1: proc[14] sys_ioctl called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_rt_sigaction called
[1]syscall1: proc[14] sys_dup3 called
[1]syscall1: proc[14] sys_close called
[1]syscall1: proc[14] sys_mmap called
[1]syscall1: proc[14] sys_execve called
[0]sys_read: [13] f->type=1, p=0x421ae4, n=1024
[0]piperead: [13] nread=0, nwrite=0, n=1024
[1]syscall1: proc[14] sys_gettid called
[1]syscall1: proc[14] sys_read called
[1]sys_read: [14] f->type=1, p=0xfffffffffac0, n=1024          // stack領域に読み込み
[1]piperead: [14] nread=0, nwrite=0, n=1024
[0]sys_read: [12] f->type=2, p=0x600000003120, n=1024          // region_mapに読み込み
[0]piperead: [13] nread=4, nwrite=4
[3]exit: [12] sort exit with 0
[0]sys_read: [13] f->type=1, p=0x421ae4, n=1024                // data領域に読み込み
[0]piperead: [13] nread=4, nwrite=94, n=1024
[0]piperead: [13] nread=94, nwrite=94
[2]piperead: [14] nread=12, nwrite=12
[0]sys_read: [13] f->type=1, p=0x421ae4, n=1024
[2]trap: iabort: esr=0x82000004, ec=32, dfs=4, far=0x41a4bc
[0]piperead: [13] nread=94, nwrite=94, n=1024
[0]piperead: [13] nread=94, nwrite=94
[2]pf_handler: pte=0xffff00003ab9e0d0, *pte=0x0
[2]trap: iabort: esr=0x82000007, ec=32, dfs=7, far=0x41a4bc
[2]pf_handler: pte=0xffff00003ab9e0d0, *pte=0x0
[2]trap: iabort: esr=0x82000007, ec=32, dfs=7, far=0x41a4bc
[2]pf_handler: pte=0xffff00003ab9e0d0, *pte=0x0
```

- vm_statを入れたらエラー発生せず

```
# sort test.txt | uniq -c | head -3
[1]execve: parse [12] /usr/bin/sort
[3]syscall1: proc[14] sys_gettid called
[3]syscall1: proc[14] sys_rt_sigprocmask called
[0]execve: parse [13] /usr/bin/uniq
[3]syscall1: proc[14] sys_rt_sigprocmask called
[3]syscall1: proc[14] sys_setpgid called
[3]syscall1: proc[14] sys_ioctl called
[3]syscall1: proc[14] sys_rt_sigaction called
[3]syscall1: proc[14] sys_rt_sigaction called
[3]syscall1: proc[14] sys_rt_sigaction called
[3]syscall1: proc[14] sys_rt_sigaction called
[3]syscall1: proc[14] sys_rt_sigaction called
[3]syscall1: proc[14] sys_dup3 called
[3]syscall1: proc[14] sys_close called
[3]syscall1: proc[14] sys_mmap called
[3]syscall1: proc[14] sys_execve called
[3]execve: parse [14] /usr/bin/head
[0]sys_read: [13] f->type=1, p=0x421ae4, n=1024
[0]piperead: [13] nread=0, nwrite=0, n=1024
[1]sys_read: [12] f->type=2, p=0x600000003120, n=1024
[3]vm_stat: va: 0x400000, pa: 0xffff00003accd000, pte: 0x3accd647, PTE_ADDR(pte): 0x3accd000, P2V(...): 0xffff00003accd000
[3]vm_stat: va: 0x401000, pa: 0xffff00003ac96000, pte: 0x3ac96647, PTE_ADDR(pte): 0x3ac96000, P2V(...): 0xffff00003ac96000
[3]vm_stat: va: 0x402000, pa: 0xffff00003ac95000, pte: 0x3ac95647, PTE_ADDR(pte): 0x3ac95000, P2V(...): 0xffff00003ac95000
[3]vm_stat: va: 0x403000, pa: 0xffff00003ac94000, pte: 0x3ac94647, PTE_ADDR(pte): 0x3ac94000, P2V(...): 0xffff00003ac94000
[3]vm_stat: va: 0x404000, pa: 0xffff00003accb000, pte: 0x3accb647, PTE_ADDR(pte): 0x3accb000, P2V(...): 0xffff00003accb000
[3]vm_stat: va: 0x405000, pa: 0xffff00003acca000, pte: 0x3acca647, PTE_ADDR(pte): 0x3acca000, P2V(...): 0xffff00003acca000
[3]vm_stat: va: 0x406000, pa: 0xffff00003acc9000, pte: 0x3acc9647, PTE_ADDR(pte): 0x3acc9000, P2V(...): 0xffff00003acc9000
[3]vm_stat: va: 0x407000, pa: 0xffff00003abc8000, pte: 0x3abc8647, PTE_ADDR(pte): 0x3abc8000, P2V(...): 0xffff00003abc8000
[3]vm_stat: va: 0x408000, pa: 0xffff00003abc7000, pte: 0x3abc7647, PTE_ADDR(pte): 0x3abc7000, P2V(...): 0xffff00003abc7000
[3]vm_stat: va: 0x409000, pa: 0xffff00003abc6000, pte: 0x3abc6647, PTE_ADDR(pte): 0x3abc6000, P2V(...): 0xffff00003abc6000
[3]vm_stat: va: 0x40a000, pa: 0xffff00003abc5000, pte: 0x3abc5647, PTE_ADDR(pte): 0x3abc5000, P2V(...): 0xffff00003abc5000
[3]vm_stat: va: 0x40b000, pa: 0xffff00003abc4000, pte: 0x3abc4647, PTE_ADDR(pte): 0x3abc4000, P2V(...): 0xffff00003abc4000
[3]vm_stat: va: 0x40c000, pa: 0xffff00003abc3000, pte: 0x3abc3647, PTE_ADDR(pte): 0x3abc3000, P2V(...): 0xffff00003abc3000
[3]vm_stat: va: 0x40d000, pa: 0xffff00003abc2000, pte: 0x3abc2647, PTE_ADDR(pte): 0x3abc2000, P2V(...): 0xffff00003abc2000
[0]piperead: [13] nread=4, nwrite=4
[3]vm_stat: va: 0x40e000, pa: 0xffff00003abc1000, pte: 0x3abc1647, PTE_ADDR(pte): 0x3abc1000, P2V(...): 0xffff00003abc1000
[3]vm_stat: va: 0x40f000, pa: 0xffff00003abc0000, pte: 0x3abc0647, PTE_ADDR(pte): 0x3abc0000, P2V(...): 0xffff00003abc0000
[1]exit: [12] sort exit with 0
[3]vm_stat: va: 0x410000, pa: 0xffff00003abbf000, pte: 0x3abbf647, PTE_ADDR(pte): 0x3abbf000, P2V(...): 0xffff00003abbf000
[3]vm_stat: va: 0x411000, pa: 0xffff00003abbe000, pte: 0x3abbe647, PTE_ADDR(pte): 0x3abbe000, P2V(...): 0xffff00003abbe000
[0]sys_read: [13] f->type=1, p=0x421ae4, n=1024
[0]piperead: [13] nread=4, nwrite=94, n=1024
[0]piperead: [13] nread=94, nwrite=94
[3]vm_stat: va: 0x412000, pa: 0xffff00003abbd000, pte: 0x3abbd647, PTE_ADDR(pte): 0x3abbd000, P2V(...): 0xffff00003abbd000
[3]vm_stat: va: 0x413000, pa: 0xffff00003abbc000, pte: 0x3abbc647, PTE_ADDR(pte): 0x3abbc000, P2V(...): 0xffff00003abbc000
[3]vm_stat: va: 0x414000, pa: 0xffff00003abbb000, pte: 0x3abbb647, PTE_ADDR(pte): 0x3abbb000, P2V(...): 0xffff00003abbb000
[3]vm_stat: va: 0x415000, pa: 0xffff00003abba000, pte: 0x3abba647, PTE_ADDR(pte): 0x3abba000, P2V(...): 0xffff00003abba000
[3]vm_stat: va: 0x416000, pa: 0xffff00003abb9000, pte: 0x3abb9647, PTE_ADDR(pte): 0x3abb9000, P2V(...): 0xffff00003abb9000
[3]vm_stat: va: 0x417000, pa: 0xffff00003abb8000, pte: 0x3abb8647, PTE_ADDR(pte): 0x3abb8000, P2V(...): 0xffff00003abb8000
[3]vm_stat: va: 0x418000, pa: 0xffff00003abb7000, pte: 0x3abb7647, PTE_ADDR(pte): 0x3abb7000, P2V(...): 0xffff00003abb7000
[0]sys_read: [13] f->type=1, p=0x421ae4, n=1024
[3]vm_stat: va: 0x419000, pa: 0xffff00003abb6000, pte: 0x3abb6647, PTE_ADDR(pte): 0x3abb6000, P2V(...): 0xffff00003abb6000
[0]piperead: [13] nread=94, nwrite=94, n=1024
[0]piperead: [13] nread=94, nwrite=94
[3]vm_stat: va: 0x41a000, pa: 0xffff00003abb5000, pte: 0x3abb5647, PTE_ADDR(pte): 0x3abb5000, P2V(...): 0xffff00003abb5000
[3]vm_stat: va: 0x41b000, pa: 0xffff00003abb4000, pte: 0x3abb4647, PTE_ADDR(pte): 0x3abb4000, P2V(...): 0xffff00003abb4000
[3]vm_stat: va: 0x41c000, pa: 0xffff00003abb3000, pte: 0x3abb3647, PTE_ADDR(pte): 0x3abb3000, P2V(...): 0xffff00003abb3000
[3]vm_stat: va: 0x41d000, pa: 0xffff00003abb2000, pte: 0x3abb2647, PTE_ADDR(pte): 0x3abb2000, P2V(...): 0xffff00003abb2000
[3]vm_stat: va: 0x41e000, pa: 0xffff00003abb1000, pte: 0x3abb1647, PTE_ADDR(pte): 0x3abb1000, P2V(...): 0xffff00003abb1000
[3]vm_stat: va: 0x41f000, pa: 0xffff00003acdf000, pte: 0x3acdf647, PTE_ADDR(pte): 0x3acdf000, P2V(...): 0xffff00003acdf000
[3]vm_stat: va: 0x420000, pa: 0xffff00003acde000, pte: 0x3acde647, PTE_ADDR(pte): 0x3acde000, P2V(...): 0xffff00003acde000
[3]vm_stat: va: 0x421000, pa: 0xffff00003acdd000, pte: 0x3acdd647, PTE_ADDR(pte): 0x3acdd000, P2V(...): 0xffff00003acdd000
[3]vm_stat: va: 0x422000, pa: 0xffff00003aea8000, pte: 0x3aea8647, PTE_ADDR(pte): 0x3aea8000, P2V(...): 0xffff00003aea8000
[3]vm_stat: va: 0xffffffff6000, pa: 0xffff00003ab99000, pte: 0x3ab99647, PTE_ADDR(pte): 0x3ab99000, P2V(...): 0xffff00003ab99000
[3]vm_stat: va: [0x400000 ~ 0x423000)
[3]vm_stat: va: 0xffffffff7000, pa: 0xffff00003ab98000, pte: 0x3ab98647, PTE_ADDR(pte): 0x3ab98000, P2V(...): 0xffff00003ab98000
[3]vm_stat: va: 0xffffffff8000, pa: 0xffff00003ab96000, pte: 0x3ab96647, PTE_ADDR(pte): 0x3ab96000, P2V(...): 0xffff00003ab96000
[3]vm_stat: va: 0xffffffff9000, pa: 0xffff00003ab93000, pte: 0x3ab93647, PTE_ADDR(pte): 0x3ab93000, P2V(...): 0xffff00003ab93000
[2]exit: [13] uniq exit with 0
[3]vm_stat: va: 0xffffffffa000, pa: 0xffff00003ab91000, pte: 0x3ab91647, PTE_ADDR(pte): 0x3ab91000, P2V(...): 0xffff00003ab91000
[3]vm_stat: va: 0xffffffffb000, pa: 0xffff00003ab90000, pte: 0x3ab90647, PTE_ADDR(pte): 0x3ab90000, P2V(...): 0xffff00003ab90000
[3]vm_stat: va: 0xffffffffc000, pa: 0xffff00003ab8f000, pte: 0x3ab8f647, PTE_ADDR(pte): 0x3ab8f000, P2V(...): 0xffff00003ab8f000
[3]vm_stat: va: 0xffffffffd000, pa: 0xffff00003ab8e000, pte: 0x3ab8e647, PTE_ADDR(pte): 0x3ab8e000, P2V(...): 0xffff00003ab8e000
[3]vm_stat: va: 0xffffffffe000, pa: 0xffff00003ab8d000, pte: 0x3ab8d647, PTE_ADDR(pte): 0x3ab8d000, P2V(...): 0xffff00003ab8d000
[3]vm_stat: va: 0xfffffffff000, pa: 0xffff00003ab9a000, pte: 0x3ab9a647, PTE_ADDR(pte): 0x3ab9a000, P2V(...): 0xffff00003ab9a000
[3]vm_stat: va: [0xffffffff6000 ~ 0x1000000000000)
[0]syscall1: proc[14] sys_gettid called
[0]syscall1: proc[14] sys_read called
[0]sys_read: [14] f->type=1, p=0xfffffffffac0, n=1024
[0]piperead: [14] nread=0, nwrite=98, n=1024
[0]piperead: [14] nread=98, nwrite=98
[0]syscall1: proc[14] sys_lseek called
[0]syscall1: proc[14] sys_ioctl called
[0]syscall1: proc[14] sys_writev called
[0]filewrite: [14] addr=0x0, n=0, f->off=92
[0]filewrite: [14] addr=0x20, n=38, f->off=92
      2 111
      2 123
      1 12345
[0]syscall1: proc[14] sys_close called
[2]syscall1: proc[14] sys_close called
[2]syscall1: proc[14] sys_close called
[0]syscall1: proc[14] sys_exit called
[0]exit: [14] head exit with 0
#
```

### メモリ共有をouterからinnerに変更

- 2回め以降の`sort test.txt | uniq -c | head -3`は何も出力されずコマンドプロンプトに戻る
- その他は特に変わりなく、正常に動く

## pipetestは相変わらずストールする

- 3つのコマンドは実行されている
- wcが結果出力している`[1]filewrite: [8] addr=0x35, n=1, f->off=9`
  - filewrite -> console_write -> unlock(ip) ->releasesleep
    wakeup -> acquire(&ptable.lock)
- 最後のacquireで&ptable.lockがデッドロック
- acquire(&ptable.lock)してrelease()していないのはexit()だけだがrelease()するとロックされていないエラーになる

### pipetest

```
[0]sleep: 'dash'(6) sleep lk=A
[0]sleep: 'dash'(6) wakeup lk=A
[3]sleep: 'dash'(6) sleep lk=B
[3]sleep: 'pipetest'(7) sleep lk=B
[1]sleep: ''(10) sleep lk=C
[0]sleep: ''(9) sleep lk=D
[1]sleep: ''(9) wakeup lk=D
[0]sleep: ''(10) wakeup lk=C
[1]piperead: [9] nread=0, nwrite=0, n=1024
[1]sleep: 'head'(9) sleep lk=E
[2]piperead: [8] nread=0, nwrite=0, n=16384
[2]sleep: 'wc'(8) sleep lk=F
[0]pipewrite: [10]: nread=0, nwrite=0, n=94, addr='1'
[2]sleep: 'head'(9) wakeup lk=E
[2]pipewrite: [9]: nread=0, nwrite=0, n=0, addr=''
[1]sleep: 'wc'(8) wakeup lk=F
[2]pipewrite: [9]: nread=0, nwrite=0, n=20, addr='1'
[1]sleep: 'wc'(8) sleep lk=F
[1]sleep: 'wc'(8) wakeup lk=F
[1]piperead: [8] nread=20, nwrite=20, n=16384
[1]sleep: 'wc'(8) sleep lk=F
[1]sleep: 'wc'(8) wakeup lk=F
[1]filewrite: [8] addr=0x35, n=1, f->off=9
```

### `cat test.txt | head -5 | wc -l`

```
[2]sleep: 'dash'(6) sleep lk=A
[3]sleep: 'dash'(6) wakeup lk=A
[1]sleep: 'dash'(6) sleep lk=B
[0]sleep: 'dash'(6) wakeup lk=B
[0]sleep: 'dash'(6) sleep lk=C
[1]sleep: 'dash'(6) wakeup lk=C
[1]sleep: 'dash'(6) sleep lk=B
[1]sleep: 'dash'(6) wakeup lk=B
[1]sleep: 'dash'(6) sleep lk=D
[1]sleep: 'dash'(6) wakeup lk=D
[1]sleep: 'dash'(6) sleep lk=E
[2]sleep: 'dash'(6) wakeup lk=E
[2]sleep: 'dash'(6) sleep lk=F
[1]sleep: ''(9) sleep lk=A
[1]sleep: ''(9) wakeup lk=A
[3]sleep: 'cat'(7) sleep lk=A
[2]sleep: 'cat'(7) wakeup lk=A
[2]pipewrite: [7]: nread=0, nwrite=0, n=94, addr='1'
[0]piperead: [8] nread=0, nwrite=94, n=1024
[0]filewrite: [8] addr=0x0, n=0, f->off=0
[0]pipewrite: [8]: nread=0, nwrite=0, n=0, addr=''
[2]sleep: 'dash'(6) wakeup lk=F
[0]filewrite: [8] addr=0x31, n=20, f->off=0
[2]sleep: 'dash'(6) sleep lk=F
[0]pipewrite: [8]: nread=0, nwrite=0, n=20, addr='1'
[0]sleep: 'dash'(6) wakeup lk=F
[0]sleep: 'dash'(6) sleep lk=F
[1]piperead: [9] nread=0, nwrite=20, n=16384
[1]piperead: [9] nread=20, nwrite=20, n=16384
5
[3]sleep: 'dash'(6) wakeup lk=F
[1]sleep: 'dash'(6) sleep lk=A
```
