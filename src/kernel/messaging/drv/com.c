// com.c
// ---------------------
// COM driver for kernew messawging sewvice :3

#include "messaging/drv/com.h"
#include "kernel_ports.h"
#include "kernel_types.h"

// featureset
com_featureset_t com_featureset = {.com1 = false, .com2 = false};

// macros
#define COM1_BASE 0x3F8
#define COM2_BASE 0x2F8
#define UART_MCR(a)  ((a) + 0x04)

// 38.4kbps
void com_set_baud(void) {
  if (com_featureset.com1) {
    outb(0x3FB, 0x80);
    outb(0x3F8, 0x03);
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03);
    outb(0x3FA, 0xC7);
    outb(UART_MCR(COM1_BASE), 0x0B);
  }
  if (com_featureset.com2) {
    outb(0x2FB, 0x80); // just replace it with 2. Neat enough.
    outb(0x2F8, 0x03);
    outb(0x2F9, 0x00);
    outb(0x2FB, 0x03);
    outb(0x2FA, 0xC7);
    outb(UART_MCR(COM2_BASE), 0x0B);
  }
}
void com_send_char(char a) {
  if (com_featureset.com1) {
    if (a == '\n') {
      while ((inb(COM1_BASE + 5) & 0x20) == 0)
        ;
      outb(COM1_BASE, '\r');
    }
    // wait on UART
    while ((inb(COM1_BASE + 5) & 0x20) == 0)
      ;

    outb(COM1_BASE, a);
  }
  if (com_featureset.com2) {
    if (a == '\n') {
      while ((inb(COM2_BASE + 5) & 0x20) == 0)
        ;
      outb(COM2_BASE, '\r');
    }
    // wait on UART
    while ((inb(COM2_BASE + 5) & 0x20) == 0)
      ;

    outb(COM2_BASE, a);
  }
}
