#include "gpio.h"
#include "i2c.h"
#include "arm.h"
#include "console.h"
#include "i2c.h"
#include "rtc.h"
#include "types.h"

void
i2c_init(int div)
{
#ifdef USING_RASPI
    // GPIO pins 2 and 3 use as alt0 (BSC1)
    uint32_t selector = get32(GPFSEL0);
    selector &= ~((7 << 6) | (7 << 9)); // clear FSEL2, FSEL3
    selector |= ((4 << 6) | (4 << 9));  // set alt0
    put32(GPFSEL0, selector);

    selector = get32(GPPUDCLK0);
    selector |= ((1 << 2) | (1 << 3));

    put32(GPPUD, 0);            /* enable pins 2 and 3 */
    delay(150);
    put32(GPPUDCLK0, selector);
    delay(150);
    //put32(GPPUD, 0);            /* flush GPIO setup 1 */
    put32(GPPUDCLK0, 0);        /* flush GPIO setup 2 */

    put32(I2C_DIV_REG, div);

    rtc_init();
#endif
}

i2c_status
i2c_recv(uint8_t address, uint8_t *buf, uint32_t size)
{
    int count = 0;

    put32(I2C_A_REG, address);
    put32(I2C_C_REG, C_CLEAR);
    put32(I2C_S_REG, S_CLKT | S_ERR | S_DONE);
    put32(I2C_DLEN_REG, size);
    put32(I2C_C_REG, C_I2CEN | C_ST | C_READ);

    while (!(get32(I2C_S_REG) & S_DONE)) {
        while (get32(I2C_S_REG) & S_RXD) {
            *buf++ = get32(I2C_FIFO_REG) & 0xff;
            count++;
        }
    }

    while (count < size && (get32(I2C_S_REG) & S_RXD)) {
        *buf++ = get32(I2C_FIFO_REG) & 0xff;
        count++;
    }

    uint32_t status = get32(I2C_S_REG);

    put32(I2C_S_REG, S_DONE);

    if (status & S_ERR)
        return I2CS_ACK_ERROR;
    else if (status & S_CLKT)
        return I2CS_CLOCK_TIMEOUT;
    else if (count < size)
        return I2CS_DATA_LOSS;

    return I2CS_SUCCESS;
}

i2c_status
i2c_send(uint8_t address, uint8_t *buf, uint32_t size)
{
    int count = 0;

    put32(I2C_A_REG, address);
    put32(I2C_C_REG, C_CLEAR);
    put32(I2C_S_REG, S_CLKT | S_ERR | S_DONE);
    put32(I2C_DLEN_REG, size);
    put32(I2C_C_REG, C_I2CEN | C_ST);

    while (!(get32(I2C_S_REG) & S_DONE)) {
        while (count < size && (get32(I2C_S_REG) & S_TXD)) {
            put32(I2C_FIFO_REG, *buf++);
            count++;
        }
    }

    uint32_t status = get32(I2C_S_REG);

    put32(I2C_S_REG, S_DONE);

    if (status & S_ERR)
        return I2CS_ACK_ERROR;
    else if (status & S_CLKT)
        return I2CS_CLOCK_TIMEOUT;
    else if (count < size)
        return I2CS_DATA_LOSS;

    return I2CS_SUCCESS;
}
