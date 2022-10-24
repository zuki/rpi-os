#ifndef INC_STRING_H
#define INC_STRING_H

#include "types.h"
#include "arm.h"

static inline void *
memset(void *str, int c, size_t n)
{
    char *l = (char *)str, *r = l + n;
    for (; l != r; l ++)
        *l = c & 0xff;
    return str;
}

static inline void *
memmove(void *dst, const void *src, ssize_t n)
{
    const char *s = (const char *)src;
    char *d = (char *)dst;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0) *--d = *--s;
    } else {
        while (n-- > 0) *d++ = *s++;
    }
    return dst;
}

static inline void *
memcpy(void *dst, const void *src, ssize_t n)
{
    return memmove(dst, src, n);
}

static inline int
memcmp(const void *v1, const void *v2, ssize_t n)
{
    for (const uint8_t *s1 = (const uint8_t*)v1, *s2 = (const uint8_t*)v2;
         n-- > 0; s1++, s2++)
        if  (*s1 != *s2)
            return *s1 - *s2;
    return 0;
}

static inline int
strncmp(const char *p, const char *q, ssize_t n)
{
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    return (uint8_t)*p - (uint8_t)*q;
}

static inline char *
strncpy(char *s, const char *t, ssize_t n)
{
    char *os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
        *s++ = 0;
    return os;
}

// Like strncpy but guaranteed to NUL-terminate.
static inline char *
safestrcpy(char *s, const char *t, ssize_t n)
{
    char *os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}

static inline size_t
strlen(const char *s)
{
    size_t n;
    for (n = 0; s[n]; n++)
        ;
    return n;
}

static inline char *
strrchr(const char *s, char c)
{
    char *p = (char *)((uint64_t)s + strlen(s) - 1);
    for (; *p && p >= s; p--) {
        if (*p == c)
            return (char *)p;
    }
    return 0;
}

static inline int
strcmp(const char *p, const char *q)
{
    while (*p && *p == *q)
        p++, q++;
    return (int) ((uint8_t)*p - (uint8_t)*q);
}

static inline void *
memscan(void *addr, int c, size_t size)
{
    unsigned char *a = addr;

    while (size) {
        if (*a == c)
            return (void *)a;
        a++;
        size--;
    }
    return (void *)a;
}

static inline void format(uint8_t c, char *sc) {
    const char digit[] = "0123456789abcdef";
    sc[0] = digit[c >> 4];
    sc[1] = digit[c & 0xf];
}

int sprintf(char *str, const char *fmt, ...);

#endif
