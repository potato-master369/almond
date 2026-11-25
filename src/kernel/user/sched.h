#ifndef USER_SCHED_H
#define USER_SCHED_H
#include "kernel_types.h"
struct __attribute__((packed)) thread_control_block {
  uint32_t pid;
  void *esp;
  void *stack_base;
  void *esp0;
  void *cr3;
  struct thread_control_block *next;
  uint8_t state;
};

struct tss_entry {
  uint32_t prev_tss;

  uint32_t esp0;
  uint32_t ss0;

  uint32_t esp1;
  uint32_t ss1;

  uint32_t esp2;
  uint32_t ss2;

  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;

  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;

  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;

  uint32_t ldt;

  uint16_t trap;
  uint16_t iomap_base;
} __attribute__((packed));
void sched_init(void);
#endif
