#include "../helpers/string.h"
#include "../kernel_ports.h"
#include "../kernel_types.h"
#include "../messaging/messaging.h"
// swap VGA text mode to VBE
#include "../framebuffer/bsman.h"
#include "../framebuffer/fb.h"
#include "pic.h"
extern fb_surface_t fb0;
extern const uint8_t terminus_fon[128][16];

// ISR callback registration
typedef void (*idt_callback_t)(uint8_t);
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
idt_callback_t isr_callbacks[16];

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

void idt_vga_write_char(char a, uint16_t cur_x, uint16_t cur_y,
                        uint32_t col) {
  for (uint16_t y = 0; y < 16; ++y) {
    uint8_t row = terminus_fon[a][y];
    for (uint16_t x = 0; x < 8; ++x) {
      if (!(row & (1 << x)))
        continue;
      fb0.write_pixel(cur_x + x, cur_y + y, col, &fb0);
    }
  }
}

uint16_t idt_vga_cur_x = 0;
uint16_t idt_vga_cur_y = 0;
void idt_vga_write(const char *s) {
  int i = 0;
  while (s[i] != '\0') {
    if (s[i] == '\n') {
      idt_vga_cur_x = 0;
      idt_vga_cur_y += 16;
    } else if (s[i] >= 0x20 && s[i] <= 0x7E) {
      if (idt_vga_cur_x >= fb0.width - 8) {
        idt_vga_cur_x = 0;
        idt_vga_cur_y += 16;
      }
      idt_vga_write_char(s[i], idt_vga_cur_x, idt_vga_cur_y, 0xffffffff);
      idt_vga_cur_x += 8;
    }
    ++i;
  }
}
// cli is scammer, so put dis here so no scam
static volatile int in_panic = 0;

void idt_register_isr(idt_callback_t f, uint8_t isr_n) {
  isr_callbacks[isr_n] = f;
}

void idt_c_handler(idt_registers_t *a) {
  char reportbuf[21];
  if (a->int_no < 0x20) {
    __asm__ __volatile__("cli");

    if (in_panic) {
      while (1) {
        __asm__ __volatile__("hlt");
      }
    }
    in_panic = 1;

    for (uint16_t x = 0; x < fb0.width; ++x) {
      for (uint16_t y = 0; y < fb0.height; ++y) {
        fb0.write_pixel(x, y, 0x001e7bff, &fb0);
      }
    }
    idt_vga_cur_x = 0;
    idt_vga_cur_y = 0;

    char numbuf[64];
    idt_vga_write("Almond Kernel\n\nCompile time: " __DATE__ " " __TIME__ "\n\n");
    idt_vga_write("  STOP - (ISR) - int 0x");
    itoa_hex(a->int_no, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("\n");
    idt_vga_write("\n\nRegisters:\n");

    idt_vga_write("EIP: 0x");
    itoa_hex(a->eip, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("  CS: 0x");
    itoa_hex(a->cs, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("  FLAGS: 0x");
    itoa_hex(a->eflags, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("\n");

    idt_vga_write("EAX: 0x");
    itoa_hex(a->eax, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("  EBX: 0x");
    itoa_hex(a->ebx, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("  ECX: 0x");
    itoa_hex(a->ecx, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("\n");

    idt_vga_write("EDX: 0x");
    itoa_hex(a->edx, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("  ESI: 0x");
    itoa_hex(a->esi, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("  EDI: 0x");
    itoa_hex(a->edi, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("\n");

    idt_vga_write("ESP: 0x");
    itoa_hex(a->esp, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("  EBP: 0x");
    itoa_hex(a->ebp, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("\n");

    idt_vga_write("ERR_CODE: 0x");
    itoa_hex(a->err_code, numbuf);
    idt_vga_write(numbuf);
    idt_vga_write("\n\n    The kernel has encountered an unrecoverable error upon recieving an interrupt, likely caused by a division by 0, invalid op or similarly. The kernel has been halted.\n\n Fault at: ??:?? (File cannot be discerned. Refer to EIP)\n\n");
    switch (a->int_no) {
    case 0:
        // STATUS_INTEGER_DIVIDE_BY_ZERO (#DE)
        idt_vga_write("    The kernel has attempted to perform a division by 0. (#DE). This is probably because something in your computer is too fast, or terrible programming.\n\n");
        break;
    case 1:
        // STATUS_SINGLE_STEP (#DB)
        idt_vga_write("    The kernel has performed a single instruction in single-step mode. This is not your fault. Report this to the developers.\n\n");
        break;
    case 2:
        // Non-Maskable Interrupt (NMI)
        idt_vga_write("    The kernel has encountered a high-priority hardware interrupt that could not be masked. Something is wrong. This could be a critical hardware failure, imminent power loss, or watchdog, or something else.\n\n")
;        break;
    case 3:
        // STATUS_BREAKPOINT (#BP)
        idt_vga_write("    The kernel has hit a breakpoint.\n\n");
        break;
    case 4:
        // STATUS_INTEGER_OVERFLOW (#OF)
        idt_vga_write("    The kernel has overflowed an integer somewhere. Idk\n\n");
        break;
    case 5:
        // STATUS_ARRAY_BOUNDS_EXCEEDED (#BR)
        idt_vga_write("    The kernel has accessed an array out of bounds.\n\n");
        break;
    case 6:
        // STATUS_ILLEGAL_INSTRUCTION (#UD)
        idt_vga_write("    The kernel has performed an illegal operation and has beeen shut down. (holy windows reference) Is your CPU too ancient, too new, or running random code?\n\n");
        break;
    case 7:
        // STATUS_FLOAT_MULTIPLE_FAULTS (#NM)
        idt_vga_write("    STATUS_FLOAT_MULTILE_FAULTS. Idk what that means. Search it up or something.\n\n");
        break;
    case 8:
        // STATUS_DOUBLE_FAULT (#DF)
        idt_vga_write("    This interrut handler has faulted before. How is that even possible.\n\n");
        break;
    case 9:
        // Coprocessor Segment Overrun
        idt_vga_write("    Coprocessor Segment Overrun. Ancient ass CPU problems. What are you running this on? A 386??\n\n");
        break;
    case 10:
        // STATUS_INVALID_TSS (#TS)
        idt_vga_write("    Invalid TSS state. Seeing this means the multitasking is terribly wrong.\n\n");
        break;
    case 11:
        // STATUS_SEGMENT_NOT_PRESENT (#NP)
        idt_vga_write("    Status segment not present. What does that even mean?\n\n");
        break;
    case 12:
        // STATUS_STACK_OVERFLOW (#SS)
        idt_vga_write("    Stack overflow. No, not the forum, but the kernel stack (which is tiny as hell btw) has been overrun. Crappy drivers!\n\n");
        break;
    case 13:
        // STATUS_ACCESS_VIOLATION (#GP)
        idt_vga_write("    Status access violation. Basically permission denied but the CPU now hates you.\n\n");
        break;
    case 14:
        // STATUS_PAGE_FAULT (#PF)
        idt_vga_write("    Page fault. Either VMM is tweaking or you are trying to access memory that isn't mapped or can't be read.\n\n");
        break;
    case 15:
        // Reserved
        break;
    case 16:
        // STATUS_FLOAT_INVALID_OPERATION (#MF)
        idt_vga_write("    Invalid FP operation. I told you computers suck at decimal points.\n\n");
        break;
    case 17:
        // STATUS_DATATYPE_MISALIGNMENT (#AC)
        idt_vga_write("     DATATYPE MISALIGNMENT! I LOVE COMPILER PROBLEMS!!!!!!!!!!!!!!!!!!!\n\n");
        break;
    case 18:
        // Machine Check Exception (#MC)
        idt_vga_write("    Machine check exception. Contrary the name, not very fun.\n\n");
        break;
    case 19:
        // STATUS_FLOAT_INEXACT_RESULT (#XF)
        idt_vga_write("    STATUS_FLOAT_INEXACT_RESULT. Idk what that means either.\n\n");
        break;
    default:
        break;
}
    fb0.update_fb(&fb0);
    while (1) {
      __asm__ __volatile__("hlt");
    }
  }

  if (a->int_no >= 0x20) {
    if (a->int_no >= 32 && a->int_no <= 47) {
      if (isr_callbacks[a->int_no - 32] != NULL) {
        isr_callbacks[a->int_no - 32](a->int_no - 32);  // fire IRQ callback
      }                                                 // send IRQ no, back relative
    }
    pic_send_eoi((uint8_t)(a->int_no - 0x20)); // send EOI
  }
}

#define IDT_GATE_REG(n) idt_set_gate(n, (uint32_t)isr##n, 0x08, 0x8E)
void idt_init(void) {
  message_send_message(" idt: init\n");
  pic_remap(0x20, 0x28); // prevent conflicts
  pic_disable();
  // init ISR callbacks
  for (int i = 0; i < 16; ++i) {
    isr_callbacks[i] = NULL;
  }
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
