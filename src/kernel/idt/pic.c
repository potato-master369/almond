// pic.c
// ------------------------------
// handles 8259 PIC
#include "../kernel_types.h"
#include "../kernel_ports.h"

// defs
#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20
#define ICW1_ICW4 0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10

#define ICW4_8086 0x01 // 8086 mode
#define ICW4_AUTO 0x02 // auto EOI
#define ICW4_BUFF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM 0x10

#define CASCADE_IRQ 2

void pic_send_eoi(uint8_t irq) {
  if (irq >= 8) 
    outb(PIC2_COMMAND, PIC_EOI);
  
  outb(PIC1_COMMAND, PIC_EOI);
}

void pic_remap(int offset_1, int offset_2) {
  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC1_DATA, offset_1);
  io_wait();
  outb(PIC2_DATA, offset_2);
  io_wait();
  outb(PIC1_DATA, 1 << CASCADE_IRQ);
  io_wait();
  outb(PIC2_DATA, CASCADE_IRQ);
  io_wait();
  
  // make them use 8086 mode, not 8080
  outb(PIC1_DATA, CASCADE_IRQ);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  // unmask the PICs
  outb(PIC1_DATA, 0);
  outb(PIC2_DATA, 0);
}

// we prefer apic, so stupid pic is useless
void pic_disable(void) {
  outb(PIC1_DATA, 0xff);
  outb(PIC2_DATA, 0xff);
}
