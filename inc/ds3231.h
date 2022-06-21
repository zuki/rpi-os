#ifndef INC_DS3231_H
#define INC_DS33231_H

#define CORE_CLOCK_SPEED   1400000000      // 1.4 GHz

#define DS3231_I2C_ADDRESS      0x68
#define DS3231_I2C_FREQUENCY    400000
#define DS3231_I2C_DIV          (CORE_CLOCK_SPEED / DS3231_I2C_FREQUENCY)

/* RTC registers don't differ much, except for the century flag */
#define DS3231_SECS_REG     0x00  /* 00-59 */
#define DS3231_MIN_REG      0x01  /* 00-59 */
#define DS3231_HOUR_REG     0x02  /* 00-23, or 1-12{am,pm} */
#  define DS3231_12HR_BIT       0x40  /* in REG_HOUR */
#  define DS3231_PM_BIT         0x20  /* in REG_HOUR */
#define DS3231_WDAY_REG     0x03  /* 01-07 */
#define DS3231_MDAY_REG     0x04  /* 01-31 */
#define DS3231_MONTH_REG    0x05  /* 01-12 */
#  define DS3231_CENTURY_BIT    0x80  /* in REG_MONTH */
#define DS3231_YEAR_REG     0x06  /* 00-99 */

/*
 * Other registers (control, status, alarms, trickle charge, NVRAM, etc)
 * start at 7, and they differ a LOT. Only control and status matter for
 * basic RTC date and time functionality; be careful using them.
 */
#define DS3231_CONTROL_REG  0x0e
#  define DS3231_nEOSC_BIT      0x80
#  define DS3231_BBSQW_BIT      0x40
#  define DS3231_CONV_BIT       0x20
#  define DS3231_RS2_BIT        0x10
#  define DS3231_RS1_BIT        0x08
#  define DS3231_INTCN_BIT      0x04
#  define DS3231_A2IE_BIT       0x02
#  define DS3231_A1IE_BIT       0x01
#define DS1337_STATUS_REG   0x0f
#  define DS3231_OSF_BIT        0x80
#  define DS3231_EN32KHZ_BIT    0x08
#  define DS3231_BSY_BIT        0x04
#  define DS3231_A2F_BIT        0x02
#  define DS3231_A1F_BIT        0x01

struct rtc_time;

void ds3231_init(void);
int ds3231_get_datetime(struct rtc_time *t);
int ds3231_set_datetime(struct rtc_time *t);

#endif
