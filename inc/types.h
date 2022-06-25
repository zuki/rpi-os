#ifndef INC_TYPES_H
#define INC_TYPES_H

#define NULL    ((void *)0)

#define CHAR_BIT    8
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define SHRT_MIN    (-1-0x7fff)
#define SHRT_MAX    0x7fff
#define USHRT_MAX   0xffff
#define INT_MIN     (-1-0x7fffffff)
#define INT_MAX     0x7fffffff
#define UINT_MAX    0xffffffffU
#define LONG_MIN    (-LONG_MAX-1)
#define LONG_MAX    0x7fffffffffffffffL
#define ULONG_MAX   (2UL*LONG_MAX+1)
#define LLONG_MIN   (-LLONG_MAX-1)
#define LLONG_MAX   0x7fffffffffffffffLL
#define ULLONG_MAX  (2ULL*LLONG_MAX+1)

typedef char            int8_t;
typedef unsigned char   uint8_t;
typedef short           int16_t;
typedef unsigned short  uint16_t;
typedef int             int32_t;
typedef unsigned int    uint32_t;
typedef long            int64_t;
typedef unsigned long   uint64_t;

typedef int64_t         ssize_t;
typedef uint64_t        size_t;
typedef uint64_t        dev_t;
typedef uint64_t        ino_t;
typedef uint32_t        mode_t;
typedef uint32_t        nlink_t;
typedef int32_t         pid_t;
typedef uint32_t        uid_t;
typedef uint32_t        gid_t;
typedef int64_t         off_t;
typedef int32_t         blksize_t;
typedef int64_t         blkcnt_t;
typedef int64_t         time_t;
typedef int64_t         suseconds_t;
typedef uint32_t        tcflag_t;
typedef uint32_t        speed_t;
typedef uint8_t         cc_t;
typedef int32_t         clockid_t;
typedef int64_t         ino64_t;
typedef off_t           off64_t;
typedef uint32_t        kernel_cap_t;

/* Efficient min and max operations */
#define MIN(_a, _b)                 \
({                                  \
    typeof(_a) __a = (_a);          \
    typeof(_b) __b = (_b);          \
    __a <= __b ? __a : __b;         \
})
#define MAX(_a, _b)                 \
({                                  \
    typeof(_a) __a = (_a);          \
    typeof(_b) __b = (_b);          \
    __a >= __b ? __a : __b;         \
})


/* Round down and up to the nearest multiple of n */
#define ROUNDDOWN(a, n)                 \
({                                      \
    uint64_t __a = (uint64_t) (a);      \
    (typeof(a)) (__a - __a % (n));      \
})
#define ROUNDUP(a, n)                                           \
({                                                              \
    uint64_t __n = (uint64_t) (n);                              \
    (typeof(a)) (ROUNDDOWN((uint64_t) (a) + __n - 1, __n));     \
})

#define ALIGN(p, n) (((p) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))
#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define IS_POWER_OF_2(n)    (!((n) & ((n)-1)))

/* Efficient min and max operations */
#define MIN(_a, _b)                 \
({                                  \
    typeof(_a) __a = (_a);          \
    typeof(_b) __b = (_b);          \
    __a <= __b ? __a : __b;         \
})
#define MAX(_a, _b)                 \
({                                  \
    typeof(_a) __a = (_a);          \
    typeof(_b) __b = (_b);          \
    __a >= __b ? __a : __b;         \
})

#define bit_add(p, n) ((p) |= (1U << (n)))
#define bit_remove(p, n) ((p) &= ~(1U << (n)))
#define bit_test(p, n) ((p) & (1U << (n)))

typedef __builtin_va_list va_list;

#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(d,s)    __builtin_va_copy(d,s)

#endif
