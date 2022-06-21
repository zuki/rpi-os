#ifndef INC_I2C_H
#define INC_I2C_H

#include "base.h"
#include "types.h"

#define I2C_BASE        (MMIO_BASE + 0x00804000)

#define I2C_C_REG       (I2C_BASE + 0x00)
#define I2C_S_REG       (I2C_BASE + 0x04)
#define I2C_DLEN_REG    (I2C_BASE + 0x08)
#define I2C_A_REG       (I2C_BASE + 0x0C)
#define I2C_FIFO_REG    (I2C_BASE + 0x10)
#define I2C_DIV_REG     (I2C_BASE + 0x14)
#define I2C_DEL_REG     (I2C_BASE + 0x18)
#define I2C_CLKT_REG    (I2C_BASE + 0x1C)

// Control register
#define C_I2CEN			(1 << 15)
#define C_INTR			(1 << 10)
#define C_INTT			(1 << 9)
#define C_INTD			(1 << 8)
#define C_ST			(1 << 7)
#define C_CLEAR			(1 << 5)
#define C_READ			(1 << 0)

// Status register
#define S_CLKT			(1 << 9)
#define S_ERR			(1 << 8)
#define S_RXF			(1 << 7)
#define S_TXE			(1 << 6)
#define S_RXD			(1 << 5)
#define S_TXD			(1 << 4)
#define S_RXR			(1 << 3)
#define S_TXW			(1 << 2)
#define S_DONE			(1 << 1)
#define S_TA			(1 << 0)

#define I2C_READBIT          0x01

typedef enum _i2c_status {
    I2CS_SUCCESS = 0,
    I2CS_ACK_ERROR = -1,
    I2CS_DATA_LOSS = -2,
    I2CS_CLOCK_TIMEOUT = -3
} i2c_status;

void i2c_init();
i2c_status i2c_recv(uint8_t address, uint8_t *buf, uint32_t size);
i2c_status i2c_send(uint8_t address, uint8_t *buf, uint32_t size);

#endif
