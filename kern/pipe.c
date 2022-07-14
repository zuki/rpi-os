#include "pipe.h"
#include "types.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "file.h"
#include "mm.h"
#include "console.h"

int
pipealloc(struct file **f0, struct file **f1, int flags)
{
    struct pipe *pi;

    pi = 0;
    *f0 = *f1 = 0;
    if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
        goto bad;
    if ((pi = (struct pipe *)kalloc()) == 0)
        goto bad;
    pi->readopen = 1;
    pi->writeopen = 1;
    pi->nwrite = 0;
    pi->nread = 0;
    initlock(&pi->lock, "pipe");
    (*f0)->type = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe = pi;
    (*f0)->flags = O_RDONLY | (flags & PIPE2_FLAGS);
    (*f0)->off = 0;
    (*f1)->type = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe = pi;
    (*f1)->flags = O_WRONLY | O_APPEND | (flags & PIPE2_FLAGS);
    (*f1)->off = 0;
    trace("pi->nread; 0x%p, nwrite: 0x%p", &pi->nread, &pi->nwrite);
    return 0;

  bad:
    if (pi)
        kfree((char *)pi);
    if (*f0)
        fileclose(*f0);
    if (*f1)
        fileclose(*f1);
    return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
    acquire(&pi->lock);
    if (writable) {
        pi->writeopen = 0;
        wakeup(&pi->nread);
    } else {
        pi->readopen = 0;
        wakeup(&pi->nwrite);
    }
    if (pi->readopen == 0 && pi->writeopen == 0) {
        release(&pi->lock);
        kfree((char *)pi);
    } else {
        release(&pi->lock);
    }
}

ssize_t
pipewrite(struct pipe *pi, char *addr, ssize_t n)
{
    ssize_t i/* = 0 */;
    struct proc *p = thisproc();

    trace("[%d]: nread=%d, nwrite=%d, n=%lld, addr='%c'", p->pid, pi->nread, pi->nwrite, n, addr[0]);
    acquire(&pi->lock);
    for (i = 0; i < n; i++) {
        while (pi->nwrite == pi->nread + PIPESIZE) {
            if (pi->readopen == 0 || p->killed) {
                release(&pi->lock);
                return -1;
            }
            wakeup(&pi->nread);
            sleep(&pi->nwrite, &pi->lock);
        }
        pi->data[pi->nwrite++ % PIPESIZE] = addr[i];
    }
    wakeup(&pi->nread);
    release(&pi->lock);
    return i;
}

ssize_t
piperead(struct pipe *pi, char *addr, ssize_t n)
{
    ssize_t i;
    struct proc *p = thisproc();
    trace("[%d] nread=%d, nwrite=%d, n=%lld", p->pid, pi->nread, pi->nwrite, n);
    acquire(&pi->lock);
    while (pi->nread == pi->nwrite && pi->writeopen) {
        if (p->killed) {
            release(&pi->lock);
            return -1;
        }
        sleep(&pi->nread, &pi->lock);
    }
    for (i = 0; i < n; i++) {
        if (pi->nread == pi->nwrite)
            break;
        addr[i] = pi->data[pi->nread++ % PIPESIZE];
    }
    wakeup(&pi->nwrite);
    release(&pi->lock);
    return i;
}
