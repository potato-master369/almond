#ifndef ALMOND_CPU_H
#define ALMOND_CPU_H
#include "kernel_types.h"
#include "mmu/vmm.h"

#define CACHE_LINE_SIZE 64
#define MAX_INTERRUPT_VECTORS 256

typedef struct _almond_thread {
  uint32_t pid;
  char name[256];
} almond_thread_t;

typedef struct _almond_cpu {
  uint32_t cpu_id;
  uint32_t apic_id;
  uint32_t core_id;
  uint32_t socket_id;

  char mfg[13];
  char prettyname[49];

  __attribute__((aligned(CACHE_LINE_SIZE)))
  uintptr_t tss_address;
  uintptr_t kernel_stack;
  vmm_pagemap_t *active_pgmap;

  __attribute__((aligned(CACHE_LINE_SIZE)))
  almond_thread_t *current_thread;
  almond_thread_t *idle_thread;
  void *scheduler_queue;

  bool_t is_idle;
  bool_t need_resched;

  __attribute__((aligned(CACHE_LINE_SIZE)))
  volatile uint32_t irq_nesting_level;
  uint32_t lock_nesting_level;
  bool_t interrupts_enabled;

  uint64_t interrupts_count[MAX_INTERRUPT_VECTORS];

  // some diagnostics
  __attribute__((aligned(CACHE_LINE_SIZE)))
  uint64_t context_switches;
  uint64_t cycles_spent_idle;
} almond_cpu_t;

void cpu_init(void);
#endif
