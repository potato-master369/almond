// com.c
// ---------------------
// COM driver for kernew messawging sewvice :3

#include "../../kernel_types.h"
#include "../../kernel_ports.h"

// macros
#define COM1_BASE 0x3F8

void com_set_baud(void) {
    outb(0x3FB, 0x80);
    outb(0x3F8, 0x03);
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03);
    outb(0x3FA, 0xC7);
}
void com_send_char(char a) {
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
