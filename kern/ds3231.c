#include "arm.h"
#include "base.h"
#include "i2c.h"
#include "i2c.h"
#include "console.h"
#include "ds3231.h"
#include "errno.h"
#include "rtc.h"
#include "types.h"

static inline uint8_t bin2bcd(int value)
{
    uint8_t msbcd = 0;

    while (value >= 10) {
        msbcd++;
        value -= 10;
    }

    return (msbcd << 4) | value;
}

static inline int bcd2bin(uint8_t value)
{
    int tens = ((int)value >> 4) * 10;
    return tens + (value & 0x0f);
}

int ds3231_get_datetime(struct rtc_time *t)
{
#ifdef USING_RASPI
    uint8_t secaddr, regs[7];
    i2c_status ret;

    secaddr = DS3231_SECS_REG;
    if ((ret = i2c_send(DS3231_I2C_ADDRESS, &secaddr, 1)) != I2CS_SUCCESS) {
        warn("send error=%d\n", ret);
        return (int)ret;
    }

    if ((ret = i2c_recv(DS3231_I2C_ADDRESS, regs, 7)) != I2CS_SUCCESS) {
        warn("recv error=%d", ret);
        return (int)ret;
    }

    trace("before: secs=%x, mins=%x, hours=%x, mday=%x, mon=%x, year=%x, wday=%x",
        regs[DS3231_SECS_REG], regs[DS3231_MIN_REG], regs[DS3231_HOUR_REG], regs[DS3231_MDAY_REG],
        regs[DS3231_MONTH_REG], regs[DS3231_YEAR_REG], regs[DS3231_WDAY_REG]);

    t->tm_sec = bcd2bin(regs[DS3231_SECS_REG] & 0x7f);
    t->tm_min = bcd2bin(regs[DS3231_MIN_REG] & 0x7f);
    if (regs[DS3231_HOUR_REG] & DS3231_12HR_BIT)
        t->tm_hour = bcd2bin(regs[DS3231_HOUR_REG] & 0x1f);
    else
        t->tm_hour = bcd2bin(regs[DS3231_HOUR_REG] & 0x3f);
    t->tm_wday = bcd2bin(regs[DS3231_WDAY_REG] & 0x07) - 1;
    t->tm_mday = bcd2bin(regs[DS3231_MDAY_REG] & 0x3f);
    t->tm_mon  = bcd2bin(regs[DS3231_MONTH_REG] & 0x1f) - 1;
    t->tm_year = bcd2bin(regs[DS3231_YEAR_REG]);

    if (regs[DS3231_MONTH_REG] & DS3231_CENTURY_BIT)
        t->tm_year += 100;

    trace("after : secs=%d, mins=%d, hours=%d, mday=%d, mon=%d, year=%d, wday=%d",
        t->tm_sec, t->tm_min, t->tm_hour, t->tm_mday, t->tm_mon, t->tm_year, t->tm_wday);
    return rtc_valid_tm(t);
#else
    t->tm_sec = 23;
    t->tm_min = 31;
    t->tm_hour = 1;
    t->tm_wday = 2;
    t->tm_mday = 21;
    t->tm_mon  = 5;
    t->tm_year = 122;

    return 0;
#endif
}

int ds3231_set_datetime(struct rtc_time *t)
{
#ifdef USING_RASPI
    uint8_t regs[8];
    i2c_status ret;

    /* assume 19YY */
    if (t->tm_year > 199)
        return -EINVAL;

    regs[0] = DS3231_SECS_REG;
    regs[1] = bin2bcd(t->tm_sec);
    regs[2] = bin2bcd(t->tm_min);
    regs[3] = bin2bcd(t->tm_hour);
    regs[4] = bin2bcd(t->tm_wday + 1);
    regs[5] = bin2bcd(t->tm_mday);
    regs[6] = bin2bcd(t->tm_mon + 1);

    if (t->tm_year > 99) {
        t->tm_year -= 100;
        regs[6] |= DS3231_CENTURY_BIT;
    }
    regs[7] = bin2bcd(t->tm_year);

    trace("secs=%x, mins=%x, hours=%x, mday=%x, mon=%x, year=%x, wday=%x",
        regs[1], regs[2], regs[3], regs[5], regs[6], regs[7], regs[4]);

    if ((ret = i2c_send(DS3231_I2C_ADDRESS, regs, sizeof(regs))) != I2CS_SUCCESS) {
        warn("send2 error=%d\n", ret);
        return (int)ret;
    }
#endif

    return 0;

}

void ds3231_init()
{
#ifdef USING_RASPI
    uint8_t status_addr  = DS1337_STATUS_REG;
    uint8_t status_data;

    i2c_send(DS3231_I2C_ADDRESS, &status_addr, 1);
    i2c_recv(DS3231_I2C_ADDRESS, &status_data, 1);

    status_data &= ~DS3231_OSF_BIT;

    i2c_send(DS3231_I2C_ADDRESS, &status_addr, 1);
    i2c_send(DS3231_I2C_ADDRESS, &status_data, 1);
#endif
}
