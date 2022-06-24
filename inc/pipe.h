#ifndef INC_PIPE_H
#define INC_PIPE_H

#include "linux/fcntl.h"
#include "file.h"
#include "types.h"
#include "spinlock.h"

#define PIPESIZE 512

#define PIPE2_FLAGS (O_CLOEXEC | O_DIRECT | O_NONBLOCK)

struct pipe {
  struct spinlock lock;
  char      data[PIPESIZE];
  uint32_t  nread;      // number of bytes read
  uint32_t  nwrite;     // number of bytes written
  int       readopen;   // read fd is still open
  int       writeopen;  // write fd is still open
};

int pipealloc(struct file **f0, struct file **f1, int flags);
void pipeclose(struct pipe *p, int writable);
ssize_t pipewrite(struct pipe *p, char *addr, ssize_t n);
ssize_t piperead(struct pipe *p, char *addr, ssize_t n);

#endif
