#include "types.h"
#include "string.h"

static void sprintint(int64_t x, int base, int sign, int zero, int col, char **p)
{
    static char digit[] = "0123456789abcdef";
    static char buf[64];

    if (sign && x < 0) {
        x = -x;
        *(*p)++ = '-';
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
        *(*p)++ = buf[i];
}

static int vsnprintfmt(char *str, size_t n, const char *fmt, va_list ap)
{
    int i, c, j;
    char *s;
    char *p = str;

    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (p - str > n) return -1;
        if (c != '%') {
            *p++ = c;
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
                sprintint(va_arg(ap, int64_t), 10, 0, z, n, &p);
            else
                sprintint(va_arg(ap, uint32_t), 10, 0, z, n, &p);
            break;
        case 'd':
            if (l == 2)
                sprintint(va_arg(ap, int64_t), 10, 1, z, n, &p);
            else
                sprintint(va_arg(ap, int), 10, 1, z, n, &p);
            break;
        case 'x':
            if (l == 2)
                sprintint(va_arg(ap, int64_t), 16, 0, z, n, &p);
            else
                sprintint(va_arg(ap, uint32_t), 16, 0, z, n, &p);
            break;
        case 'p':
            sprintint((uint64_t) va_arg(ap, void *), 16, 0, 0, 16, &p);
            break;
        case 'c':
            *p++ = (va_arg(ap, int));
            break;
        case 's':
            j = 0;
            if ((s = (char*)va_arg(ap, char *)) == 0) {
                s = "(null)";
                j = 6;
            }
            for (; *s; s++) {
                *p++ = *s;
                j++;
            }
            if (n > j) {
                n = n - j;
                while(n--) {
                    *p++ = ' ';
                }
            }
            break;
        case '%':
            *p++ = '%';
            break;
        default:
            /* Print unknown % sequence to draw attention. */
            *p++ = '%';
            *p++ = c;
            break;
        }
    }
    *p = '\0';
    return 0;
}

static int vsprintfmt(char *str, const char *fmt, va_list ap)
{
    // 最大64文字
    return vsnprintfmt(str, 64, fmt, ap);
}

int sprintf(char *str, const char *fmt, ...)
{
    int ret;

    va_list ap;
    va_start(ap, fmt);
    ret = vsprintfmt(str, fmt, ap);
    va_end(ap);
    return ret;
}
