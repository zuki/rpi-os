#ifndef INC_LOG_H
#define INC_LOG_H

struct buf;

#define NLOG    3

void initlog(int dev);
void log_write(struct buf *);
void begin_op(void);
void end_op(void);

#endif
