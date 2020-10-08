#include <stdint.h>

#include "uart.h"
#include "arm.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

void
uart_putchar(int c)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x20))
        ;
    put32(AUX_MU_IO_REG, c & 0xFF);
    /* Fix Windows's '\r'. */
    if (c == '\n') uart_putchar('\r');
}

char
uart_intr()
{
    unsigned int stat;
    while (!((stat = get32(AUX_MU_IIR_REG)) & 1)) {
        /* Receiver holds a valid byte. */
        if((stat & 6) == 4) {
            int c = get32(AUX_MU_IO_REG);
            // TODO
            // rxbuffer[rxhead] = rc & 0xFF;
            // rxhead = (rxhead+1) & RXBUFMASK;
            uart_putchar(c);
        }
    }
}

void
uart_init()
{
    uint32_t selector;

    selector = get32(GPFSEL1);
    selector &= ~(7<<12);                   /* Clean gpio14. */
    selector |= 2<<12;                      /* Set alt5 for gpio14. */
    selector &= ~(7<<15);                   /* Clean gpio15. */
    selector |= 2<<15;                      /* Set alt5 for gpio15. */
    put32(GPFSEL1, selector);

    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0, 0);

    /* Enable mini uart and enable access to its registers. */
    put32(AUX_ENABLES, 1);
    /* Disable auto flow control and disable receiver and transmitter (for now). */
    put32(AUX_MU_CNTL_REG, 0);
    /* Enable receive interrupts. */
    put32(AUX_MU_IER_REG, 3 << 2 | 1);
    /* Enable 8 bit mode */
    put32(AUX_MU_LCR_REG, 3);
    /* Set RTS line to be always high. */
    put32(AUX_MU_MCR_REG, 0);
    /* Set baud rate to 115200 */
    put32(AUX_MU_BAUD_REG, 270);
    /* Clear receive and transmit FIFO. */
    put32(AUX_MU_IIR_REG, 6);
    /* Finally, enable transmitter and receiver. */
    put32(AUX_MU_CNTL_REG, 3);
}
