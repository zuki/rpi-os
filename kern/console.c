#include "console.h"
#include "types.h"
#include "arm.h"
#include "uart.h"
#include "irq.h"
#include "spinlock.h"
#include "file.h"
#include "vfs.h"
#include "mm.h"
#include "linux/termios.h"

#define CONSOLE 1

struct spinlock dbglock;
static struct spinlock conslock;
static int panicked = -1;

#define INPUT_BUF 128
struct {
    char buf[INPUT_BUF];
    size_t r;                   // Read index
    size_t w;                   // Write index
    size_t e;                   // Edit index
} input;
#define C(x)  ((x)-'@')         // Control-x
#define BACKSPACE 0x100

#define isecho (devsw[CONSOLE].termios->c_lflag & ECHO)
#define islbuf (devsw[CONSOLE].termios->c_lflag & ICANON)

static void set_default_termios(struct termios *termios)
{
    memset(termios, 0, sizeof(struct termios));

    termios->c_oflag = (OPOST|ONLCR);
    termios->c_iflag = (ICRNL|IXON|IXANY|IMAXBEL);
    termios->c_cflag = (CREAD|CS8|B115200);
    termios->c_lflag = (ISIG|ICANON|IEXTEN|ECHO|ECHOE|ECHOCTL|ECHOKE);

    termios->c_cc[VTIME]   = 0;
    termios->c_cc[VMIN]    = 1;
    termios->c_cc[VINTR]   = (0x1f & 'c');
    termios->c_cc[VQUIT]   = 28;
    termios->c_cc[VEOF]    = (0x1f & 'd');
    termios->c_cc[VSUSP]   = (0x1f & 'z');
    termios->c_cc[VKILL]   = (0x1f & 'u');
    termios->c_cc[VERASE]  = 0177;
    termios->c_cc[VWERASE] = (0x1f & 'w');
}

static void
consputc(int c)
{
    if (c == BACKSPACE) {
        uart_putchar('\b');
        uart_putchar(' ');
        uart_putchar('\b');
    } else
        uart_putchar(c);
}

static ssize_t
console_write(struct inode *ip, char *buf, ssize_t n)
{
    ip->iops->iunlock(ip);
    acquire(&conslock);
    for (size_t i = 0; i < n; i++)
        consputc(buf[i] & 0xff);
    release(&conslock);
    ip->iops->ilock(ip);
    return n;
}

static ssize_t
console_read(struct inode *ip, char *dst, ssize_t n)
{
    ip->iops->iunlock(ip);
    size_t target = n;
    acquire(&conslock);
    while (n > 0) {
        while (input.r == input.w) {
            if (thisproc()->killed) {
                release(&conslock);
                ip->iops->ilock(ip);
                return -1;
            }
            sleep(&input.r, &conslock);
        }
        int c = input.buf[input.r++ % INPUT_BUF];
        if (!islbuf) {
            release(&conslock);
            ip->iops->ilock(ip);
            *dst = c;
            return 1;
        }
        if (c == C('D')) {      // EOF
            if (n < target) {
                // Save ^D for next time, to make sure
                // caller gets a 0-byte result.
                input.r--;
            }
            break;
        }
        *dst++ = c;
        --n;
        if (c == '\n')
            break;
    }
    release(&conslock);
    ip->iops->ilock(ip);

    return target - n;
}

static void
console_intr1(int (*getc)())
{
    int c, prof = 0;

    acquire(&conslock);
    if (panicked >= 0) {
        release(&conslock);
        while (1) ;
    }

    while ((c = getc()) >= 0) {
        switch (c) {
        case C('P'):           // Process listing.
            prof = 1;
            break;
        case C('U'):           // Kill line.
            while (input.e != input.w
                   && input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
                input.e--;
                consputc(BACKSPACE);
            }
            break;
        case C('H'):
        case '\x7f':           // Backspace
            if (input.e != input.w) {
                input.e--;
                consputc(BACKSPACE);
            }
            break;
        default:
            if (c != 0 && input.e - input.r < INPUT_BUF) {
                c = (c == '\r') ? '\n' : c;
                input.buf[input.e++ % INPUT_BUF] = c;
                if (isecho) consputc(c);
                if (!islbuf || c == '\n' || c == C('D')
                    || input.e == input.r + INPUT_BUF) {
                    input.w = input.e;
                    wakeup(&input.r);
                }
            }
            break;
        }
    }
    release(&conslock);

    if (prof) {
        mm_dump();
        procdump();
    }
}

void
console_intr()
{
    console_intr1(uart_getchar);
}

void
console_init()
{
    uart_init();

    irq_enable(IRQ_AUX);
    irq_register(IRQ_AUX, console_intr);

    devsw[CONSOLE].read = console_read;
    devsw[CONSOLE].write = console_write;

    devsw[CONSOLE].termios = (struct termios *)kalloc();
    info("devsw[%d].termios: 0x%p", CONSOLE, devsw[CONSOLE].termios);
    set_default_termios(devsw[CONSOLE].termios);
}

static void
printint(int64_t x, int base, int sign, int zero, int col)
{
    static char digit[] = "0123456789abcdef";
    static char buf[64];

    if (sign && x < 0) {
        x = -x;
        uart_putchar('-');
    }

    int i = 0;
    uint64_t t = x;
    do {
        buf[i++] = digit[t % base];
    } while (t /= base);
    for (; i < col; i++)
        if (zero == 1)
            buf[i] = '0';
        else if (zero == -1)
            buf[i] = ' ';
        else break;
    while (i--)
        uart_putchar(buf[i]);
}

void
vprintfmt(void (*putch)(int), const char *fmt, va_list ap)
{
    int i, c, j;
    char *s;

    if (panicked >= 0 && panicked != cpuid()) {
        release(&conslock);
        while (1) ;
    }

    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            putch(c);
            continue;
        }

        int n = 0;
        int z = 0;
        if (fmt[i+1] == '0') {
            z = 1;
            i++;
        } else if (fmt[i+1] == '-') {
            z = -1;
            i++;
        }

        for (; fmt[i+1] >= '0' && fmt[i+1] <= '9'; i++) {
            n = n * 10 + (fmt[i+1] - '0') % 10;
        }

        int l = 0;
        for (; fmt[i + 1] == 'l'; i++)
            l++;

        if (!(c = fmt[++i] & 0xff))
            break;

        switch (c) {
        case 'u':
            if (l == 2)
                printint(va_arg(ap, int64_t), 10, 0, z, n);
            else
                printint(va_arg(ap, uint32_t), 10, 0, z, n);
            break;
        case 'd':
            if (l == 2)
                printint(va_arg(ap, int64_t), 10, 1, z, n);
            else
                printint(va_arg(ap, int), 10, 1, z, n);
            break;
        case 'x':
            if (l == 2)
                printint(va_arg(ap, int64_t), 16, 0, z, n);
            else
                printint(va_arg(ap, uint32_t), 16, 0, z, n);
            break;
        case 'p':
            printint((uint64_t) va_arg(ap, void *), 16, 0, 0, 16);
            break;
        case 'c':
            putch(va_arg(ap, int));
            break;
        case 's':
            j = 0;
            if ((s = (char*)va_arg(ap, char *)) == 0) {
                s = "(null)";
                j = 6;
            }
            for (; *s; s++) {
                putch(*s);
                j++;
            }
            if (n > j) {
                n = n - j;
                while(n--) {
                    putch(' ');
                }
            }
            break;
        case '%':
            putch('%');
            break;
        default:
            /* Print unknown % sequence to draw attention. */
            putch('%');
            putch(c);
            break;
        }
    }
}

/* Print to the console. */
void
cprintf(const char *fmt, ...)
{
    va_list ap;

    acquire(&conslock);
    va_start(ap, fmt);
    vprintfmt(uart_putchar, fmt, ap);
    va_end(ap);
    release(&conslock);
}

/* Caller should hold conslock. */
void
cprintf1(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintfmt(uart_putchar, fmt, ap);
    va_end(ap);
}


void
panic(const char *fmt, ...)
{
    va_list ap;

    acquire(&conslock);
    if (panicked < 0)
        panicked = cpuid();
    else {
        release(&conslock);
        while (1) ;
    }
    va_start(ap, fmt);
    vprintfmt(uart_putchar, fmt, ap);
    va_end(ap);
    release(&conslock);

    cprintf("%s:%d: kernel panic at cpu %d.\n", __FILE__, __LINE__,
            cpuid());
    while (1) ;
}
