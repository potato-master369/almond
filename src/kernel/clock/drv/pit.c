// pit.c
// -----------------------------
// Programmable Interrupt Timer
//
// Doesn't that sound like armpit?
#include "../../kernel_ports.h"
#include "../../idt/idt.h"
#include "../../idt/pic.h"

// This will be read to get the milliseconds
// since boot; accurate-ish
volatile uint32_t ms_since_boot;
static volatile uint32_t pit_acc; // Bresenham remainder

// Macros
#define PIT_RELOAD 1193u
#define PIT_NUM    88u // 88 Hz
#define PIT_ACCU   105000u // accumulationn units
#define TICK_STEP  (PIT_RELOAD * PIT_NUM)

/**
 * Saves the current interrupt state and disables interrupts.
 * @return The current state of the EFLAGS/RFLAGS register.
 */
static inline uint32_t irq_save(void) {
    uint32_t flags;

    // Push the flags register onto the stack, then pop it into the 'flags' variable.
    // Afterward, disable interrupts using 'cli'.
    __asm__ __volatile__(
        "pushfl \n\t"
        "popl %0 \n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );

    return flags;
}

/**
 * Restores a previously saved interrupt state.
 * @param flags The saved flags from a previous call to irq_save().
 */
static inline void irq_restore(uint32_t flags) {
    // Push the saved flags onto the stack, then pop them back into the flags register.
    __asm__ __volatile__(
        "pushl %0 \n\t"
        "popfl"
        :
        : "rm"(flags)
        : "memory", "cc"
    );
}

void pit_irq0(uint8_t irq_n) {
  if (irq_n != 0)
	  return;
  uint32_t acc = pit_acc + TICK_STEP;
  uint32_t ms = ms_since_boot;
  while (acc >= PIT_ACCU) {
    acc -= PIT_ACCU;
    ++ms;
  }
  pit_acc = acc;
  ms_since_boot = ms;
  // EOI handled in src/kernel/idt/panic.c
}

void pit_init(void) {
  uint32_t f = irq_save();
  ms_since_boot = 0;
  pit_acc = 0;
  idt_register_isr(pit_irq0, 0);
  outb(0x43, 0x34);
  outb(0x40, (uint8_t)(PIT_RELOAD & 0xFFu));
  outb(0x40, (uint8_t)(PIT_RELOAD >> 8));
  pic_enable_irq(0);
  irq_restore(f);
}

uint32_t pit_get_ms(void) {
  uint32_t f = irq_save();
  uint32_t ms = ms_since_boot;
  irq_restore(f);
  return ms;
}
