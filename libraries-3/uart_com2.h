#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sel4/sel4.h>

#define COM2_PORT_BASE 0x2f8
#define COM2_PORT_TOP  0x2ff

/* 16550A Register Offsets */
#define UART_DATA      0  /* Data R/W (DLAB=0) */
#define UART_DLL       0  /* Divisor Latch Low (DLAB=1) */
#define UART_DLH       1  /* Divisor Latch High (DLAB=1) */
#define UART_IER       1  /* Interrupt Enable Register */
#define UART_FCR       2  /* FIFO Control Register */
#define UART_LCR       3  /* Line Control Register */
#define UART_MCR       4  /* Modem Control Register */
#define UART_LSR       5  /* Line Status Register */

/* Line Status Register bits */
#define LSR_DR         0x01  /* Data Ready */
#define LSR_THRE       0x20  /* Transmitter Holding Register Empty */

static inline uint8_t uart_inb(seL4_CPtr ioport_cap, uint16_t port) {
    seL4_X86_IOPort_In8_t res = seL4_X86_IOPort_In8(ioport_cap, port);
    return res.result;
}

static inline void uart_outb(seL4_CPtr ioport_cap, uint16_t port, uint8_t val) {
    seL4_X86_IOPort_Out8(ioport_cap, port, val);
}

static inline void uart_com2_init(seL4_CPtr ioport_cap) {
    /* Disable interrupts */
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_IER, 0x00);

    /* Enable DLAB (set baud rate divisor) */
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_LCR, 0x80);

    /* Set divisor to 1 (115200 baud): DLL = 1, DLH = 0 */
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_DLL, 0x01);
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_DLH, 0x00);

    /* 8 bits, no parity, one stop bit (8N1), clear DLAB */
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_LCR, 0x03);

    /* Enable FIFO, clear TX/RX FIFOs, 14-byte threshold */
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_FCR, 0xC7);

    /* Turn on RTS and DTR */
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_MCR, 0x03);
}

static inline uint8_t uart_com2_read_byte(seL4_CPtr ioport_cap) {
    while ((uart_inb(ioport_cap, COM2_PORT_BASE + UART_LSR) & LSR_DR) == 0) {
        /* Busy-wait / poll */
    }
    return uart_inb(ioport_cap, COM2_PORT_BASE + UART_DATA);
}

static inline void uart_com2_write_byte(seL4_CPtr ioport_cap, uint8_t b) {
    while ((uart_inb(ioport_cap, COM2_PORT_BASE + UART_LSR) & LSR_THRE) == 0) {
        /* Busy-wait / poll */
    }
    uart_outb(ioport_cap, COM2_PORT_BASE + UART_DATA, b);
}

static inline void uart_com2_read_exact(seL4_CPtr ioport_cap, uint8_t *dest, size_t len) {
    for (size_t i = 0; i < len; i++) {
        dest[i] = uart_com2_read_byte(ioport_cap);
    }
}

static inline void uart_com2_write_exact(seL4_CPtr ioport_cap, const uint8_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uart_com2_write_byte(ioport_cap, src[i]);
    }
}
