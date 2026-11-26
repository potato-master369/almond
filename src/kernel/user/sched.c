// sched.c
// --------------------------------
// Scheduling idk
//
// Stolen from: https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial
#include "sched.h"
#include "context.h"
#include "helpers/string.h"
#include "kernel_types.h"
#include "messaging/messaging.h"
#include "mmu/kmalloc.h"

// macros
#define KERNEL_STACK_SIZE (2 * 1024 * 1024)

typedef void (*kernel_task_entry_t)(void);
struct tss_entry kernel_tss;

extern void task_exit_halt(void);
struct thread_control_block *current_task_TCB;
extern void switch_to_task(struct thread_control_block *next_thread);
extern void start_first_task(struct thread_control_block *c);
extern void gdt_set_tss(uint32_t base, uint32_t limit);
extern void tss_load(uint16_t selector);
struct thread_control_block *create_new_task(uint32_t pid,
                                             kernel_task_entry_t startingEIP);

// Helpers
static inline void debug_uint(char *label, uint32_t value) {
  char buf[64] = "";
  append_string(buf, label, sizeof(buf));
  append_uint(buf, value, sizeof(buf));
  append_string(buf, "\n", sizeof(buf));
  message_send_message(buf);
}
static inline uint32_t read_cr3(void) {
  uint32_t value;
  asm volatile("mov %%cr3, %0" : "=r"(value));
  return value;
}

static void task_exit(void) {}

// Replace with actual code to halt task
//  - Should stop task, remove from pool, and switch to another task without returning
__attribute__((noreturn)) void task_exit_halt(void) {
  asm volatile("cli\n"
               "1:\n"
               "hlt\n"
               "jmp 1b\n"); // spin

  __builtin_unreachable();
}

// Dummy tasks used for testing
void dummy_task(void) {
  uint32_t n = 0;
  char msgbuf[64];
  for (;;) {
    message_send_message("i am dummy task (probably pid=0)\n");
    ++n;
    strcpy(msgbuf, "n = ");
    append_uint(msgbuf, n, sizeof(msgbuf));
    append_string(msgbuf, "\n", sizeof(msgbuf));
    message_send_message(msgbuf);
    switch_to_task(current_task_TCB->next);
  }
}
void dummy_task2(void) {
  uint32_t n = 0;
  char msgbuf[64];
  for (;;) {
    message_send_message("i am dummy task (probably pid=1)\n");
    ++n;
    strcpy(msgbuf, "n = ");
    append_uint(msgbuf, n, sizeof(msgbuf));
    append_string(msgbuf, "\n", sizeof(msgbuf));
    message_send_message(msgbuf);
    switch_to_task(current_task_TCB->next);
  }
}


void tss_init(void) {
  memset(&kernel_tss, 0, sizeof(kernel_tss));

  kernel_tss.ss0 = 0x10; // Your kernel data selector
  kernel_tss.esp0 = (uint32_t)0xC9F00000;

  kernel_tss.iomap_base = sizeof(struct tss_entry);

  gdt_set_tss((uint32_t)&kernel_tss, sizeof(struct tss_entry) - 1);

  tss_load(0x18);
}


void sched_init(void) {
  current_task_TCB = create_new_task(0, dummy_task);
  current_task_TCB->next = create_new_task(1, dummy_task2);
  current_task_TCB->next->next = current_task_TCB;
  start_first_task(current_task_TCB); // cooperative
  message_send_message(" sched: init\n");
}

void task_startup(kernel_task_entry_t entry) {
  entry();
  task_exit();
}

struct thread_control_block *create_new_task(uint32_t pid,
                                             kernel_task_entry_t startingEIP) {
  struct thread_control_block *f;

  f = kmalloc(sizeof(struct thread_control_block));
  if (!f)
    return NULL;

  void *stack_base = kmalloc(KERNEL_STACK_SIZE);
  if (!stack_base) {
    kfree(f);
    return NULL;
  }
  f->stack_base = stack_base;

  uintptr_t stack_top = (uintptr_t)stack_base + KERNEL_STACK_SIZE;

  stack_top &= ~(uintptr_t)0xF;

  uint32_t *stack = (uint32_t *)stack_top;

  // Construct initial cdecl stack.
  *--stack = (uint32_t)startingEIP;    // Argument
  *--stack = (uint32_t)task_exit_halt; // Fake return address
  *--stack = (uint32_t)task_startup;   // Initial EIP

  // Initial register values.
  *--stack = 0; // EBX
  *--stack = 0; // ESI
  *--stack = 0; // EDI
  *--stack = 0; // EBP

  f->pid = pid;
  f->state = 0;
  f->esp = stack;
  f->esp0 = (void *)stack_top;
  f->next = NULL;

  // TODO: Fix this when we move from flat addressing
  uint32_t cr3 = read_cr3();

  debug_uint("read_cr3 = ", cr3);

  f->cr3 = (void *)cr3;

  debug_uint("f->cr3 = ", (uint32_t)f->cr3);
  debug_uint("&f->cr3 = ", (uint32_t)&f->cr3);

  return f;
}
