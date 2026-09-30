#include "../helpers/string.h"
#include "../kernel_ports.h"
#include "../kernel_types.h"
#include "../messaging/messaging.h"
#include "pic.h"

// ISR functions
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
// IRQ 0-15
extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);

typedef struct __attribute__((packed)) {
  uint32_t ds; // Matches the 'push eax' after pushad
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pushad
  uint32_t int_no, err_code;                       // Pushed by ISR macro / CPU
  uint32_t eip, cs, eflags; // Pushed by CPU automatically
} idt_registers_t;

typedef struct __attribute__((packed)) {
  uint16_t base_low;
  uint16_t sel;
  uint8_t always0;
  uint8_t flags;
  uint16_t base_high;
} idt_entry_t;

typedef struct __attribute__((packed)) {
  uint16_t limit;
  uint32_t base;
} idt_ptr_t;

idt_entry_t idt[256];
idt_ptr_t idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt[num].base_low = base & 0xFFFF;
  idt[num].base_high = (base >> 16) & 0xFFFF;
  idt[num].sel = sel;
  idt[num].always0 = 0;
  idt[num].flags = flags;
}

void idt_install(void) {
  idtp.limit = (sizeof(idt_entry_t) * 256) - 1;
  idtp.base = (uint32_t)&idt;

  // Load the IDT into the CPU register
  __asm__ __volatile__("lidt %0" : : "m"(idtp));
}

#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_ATTR 0x1F00u // 青色上の白色
void idt_vga_write_char(char a, unsigned char cur_x, unsigned char cur_y,
                        uint16_t attr) {
  uint16_t pos = (cur_y * 80 + cur_x);
  volatile uint16_t *where = (volatile uint16_t *)0xb8000 + pos;
  *where = (uint16_t)a | (attr);
  // move cursor
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void idt_vga_scroll_up(void) {
  uint16_t *vga = (uint16_t *)0xb8000;
  for (int i = 0; i < 24 * 80; ++i) {
    vga[i] = vga[i + 80];
  }
  uint16_t blank = VGA_ATTR | ' ';
  for (int i = 24 * 80; i < 25 * 80; ++i) {
    vga[i] = blank;
  }
}
uint16_t idt_vga_cur_x = 0;
uint16_t idt_vga_cur_y = 0;
void idt_vga_write(const char *s, uint16_t attr) {
  int i = 0;
  while (s[i] != '\0') {
    if (s[i] == '\n') {
      idt_vga_cur_x = 0;
      if (idt_vga_cur_y == VGA_ROWS - 1)
        idt_vga_scroll_up();
      else
        ++idt_vga_cur_y;
    } else if (s[i] >= 0x20 && s[i] <= 0x7E) {
      if (idt_vga_cur_x >= 80) {
        idt_vga_cur_x = 0;
        if (idt_vga_cur_y == VGA_ROWS - 1)
          idt_vga_scroll_up();
        else
          ++idt_vga_cur_y;
      }
      idt_vga_write_char(s[i], idt_vga_cur_x, idt_vga_cur_y, attr);
      ++idt_vga_cur_x;
    }
    ++i;
  }
}
// cli is scammer, so put dis here so no scam
static volatile int in_panic = 0;

void idt_c_handler(idt_registers_t *a) {
  char reportbuf[21];
  message_send_message(" idt: IRQ 0x");
  itoa_hex(a->int_no, reportbuf);
  message_send_message(reportbuf);
  message_send_message("\n");
  if (a->int_no < 0x20) {
    __asm__ __volatile__("cli");

    if (in_panic) {
      while (1) {
        __asm__ __volatile__("hlt");
      }
    }
    in_panic = 1;

    volatile uint16_t *vga = (volatile uint16_t *)0x000B8000;

    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) {
      vga[i] = VGA_ATTR | ' ';
    }
    idt_vga_cur_x = 0;
    idt_vga_cur_y = 0;

    char numbuf[64];
    idt_vga_write("Almond\n", VGA_ATTR);
    idt_vga_write("  Kernel Panic (ISR) - int 0x", VGA_ATTR);
    itoa_hex(a->int_no, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("\n", VGA_ATTR);
    idt_vga_write("\n\nRegisters:\n", VGA_ATTR);

    idt_vga_write("EIP: 0x", VGA_ATTR);
    itoa_hex(a->eip, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("  CS: 0x", VGA_ATTR);
    itoa_hex(a->cs, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("  FLAGS: 0x", VGA_ATTR);
    itoa_hex(a->eflags, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("\n", VGA_ATTR);

    idt_vga_write("EAX: 0x", VGA_ATTR);
    itoa_hex(a->eax, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("  EBX: 0x", VGA_ATTR);
    itoa_hex(a->ebx, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("  ECX: 0x", VGA_ATTR);
    itoa_hex(a->ecx, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("\n", VGA_ATTR);

    idt_vga_write("EDX: 0x", VGA_ATTR);
    itoa_hex(a->edx, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("  ESI: 0x", VGA_ATTR);
    itoa_hex(a->esi, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("  EDI: 0x", VGA_ATTR);
    itoa_hex(a->edi, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("\n", VGA_ATTR);

    idt_vga_write("ESP: 0x", VGA_ATTR);
    itoa_hex(a->esp, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("  EBP: 0x", VGA_ATTR);
    itoa_hex(a->ebp, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("\n", VGA_ATTR);

    idt_vga_write("ERR_CODE: 0x", VGA_ATTR);
    itoa_hex(a->err_code, numbuf);
    idt_vga_write(numbuf, VGA_ATTR);
    idt_vga_write("\n", VGA_ATTR);

    idt_vga_write(" *** HALT", VGA_ATTR);
    while (1) {
      __asm__ __volatile__("hlt");
    }
  }

  if (a->int_no >= 0x20) {
    pic_send_eoi((uint8_t)(a->int_no - 0x20));
  }
}

#define IDT_GATE_REG(n) idt_set_gate(n, (uint32_t)isr##n, 0x08, 0x8E)
void idt_init(void) {
  message_send_message(" idt: init\n");
  pic_remap(0x20, 0x28); // prevent conflicts
  pic_disable();
  IDT_GATE_REG(0);
  IDT_GATE_REG(1);
  IDT_GATE_REG(2);
  IDT_GATE_REG(3);
  IDT_GATE_REG(4);
  IDT_GATE_REG(5);
  IDT_GATE_REG(6);
  IDT_GATE_REG(7);
  IDT_GATE_REG(8);
  IDT_GATE_REG(9);
  IDT_GATE_REG(10);
  IDT_GATE_REG(11);
  IDT_GATE_REG(12);
  IDT_GATE_REG(13);
  IDT_GATE_REG(14);
  IDT_GATE_REG(15);
  IDT_GATE_REG(16);
  IDT_GATE_REG(17);
  IDT_GATE_REG(18);
  IDT_GATE_REG(32);
  IDT_GATE_REG(33);
  IDT_GATE_REG(34);
  IDT_GATE_REG(35);
  IDT_GATE_REG(36);
  IDT_GATE_REG(37);
  IDT_GATE_REG(38);
  IDT_GATE_REG(39);
  IDT_GATE_REG(40);
  IDT_GATE_REG(41);
  IDT_GATE_REG(42);
  IDT_GATE_REG(43);
  IDT_GATE_REG(44);
  IDT_GATE_REG(45);
  IDT_GATE_REG(46);
  IDT_GATE_REG(47);
  idt_install();
}
