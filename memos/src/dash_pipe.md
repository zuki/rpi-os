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
