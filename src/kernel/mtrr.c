// mtrr.c
// -------------------------
// MTRR for Almond

#include "cpuid_c.h"
#include "kernel_types.h"
#include "messaging/messaging.h"

#define MTRR_CAP 0xFE
#define MTRR_BASE 0x200
#define MTRR_MASK 0x201
#define MTRR_DEF_TYPE 0x2FF

#define MTRR_UC 0
#define MTRR_WC 1
#define MTRR_WT 4
#define MTRR_WP 5
#define MTRR_WB 6

#define MTRR_VALID (1ULL << 11)
#define MTRR_ENABLED (1ULL << 11)
#define MTRR_WC_SUPPORTED (1ULL << 10)

#define CR0_NW (1UL << 29)
#define CR0_CD (1UL << 30)
#define CR4_PGE (1UL << 7)

static bool_t mtrr_ready;
static bool_t mtrr_has_pge;
static uint32_t mtrr_count;
static uint64_t mtrr_cap;
static uint64_t mtrr_address_mask;

struct mtrr_state {
  unsigned long flags;
  unsigned long cr0;
  uint64_t def_type;
};

static inline uint64_t mtrr_rdmsr(uint32_t msr) {
  uint32_t lo, hi;

  __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr) : "memory");

  return ((uint64_t)hi << 32) | lo;
}

static inline void mtrr_wrmsr(uint32_t msr, uint64_t value) {
  __asm__ volatile("wrmsr"
                   :
                   : "c"(msr), "a"((uint32_t)value),
                     "d"((uint32_t)(value >> 32))
                   : "memory");
}

static uint32_t mtrr_extended_cpuid_eax(uint32_t leaf) {
  uint32_t eax, ebx, ecx, edx;

  __asm__ volatile("cpuid"
                   : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                   : "a"(leaf), "c"(0));

  return eax;
}

static inline unsigned long mtrr_irq_save(void) {
  unsigned long flags;

  __asm__ volatile("pushf\n\t"
                   "pop %0\n\t"
                   "cli"
                   : "=r"(flags)
                   :
                   : "memory");

  return flags;
}

static inline void mtrr_irq_restore(unsigned long flags) {
  if (flags & (1UL << 9))
    __asm__ volatile("sti" ::: "memory");
}

static void mtrr_flush_tlb(void) {
  unsigned long value;

  if (mtrr_has_pge) {
    __asm__ volatile("mov %%cr4, %0" : "=r"(value));

    __asm__ volatile("mov %0, %%cr4" : : "r"(value ^ CR4_PGE) : "memory");

    __asm__ volatile("mov %0, %%cr4" : : "r"(value) : "memory");
  } else {
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));

    __asm__ volatile("mov %0, %%cr3" : : "r"(value) : "memory");
  }
}

static void mtrr_begin(struct mtrr_state *state) {
  unsigned long cr0;

  __asm__ volatile("mov %%cr0, %0" : "=r"(state->cr0));

  state->def_type = mtrr_rdmsr(MTRR_DEF_TYPE);
  cr0 = (state->cr0 | CR0_CD) & ~CR0_NW;

  __asm__ volatile("mov %0, %%cr0\n\t"
                   "wbinvd"
                   :
                   : "r"(cr0)
                   : "memory");

  mtrr_flush_tlb();
  mtrr_wrmsr(MTRR_DEF_TYPE, state->def_type & ~MTRR_ENABLED);
}

static void mtrr_end(struct mtrr_state *state, uint64_t def_type) {
  mtrr_wrmsr(MTRR_DEF_TYPE, def_type);

  __asm__ volatile("wbinvd" ::: "memory");
  mtrr_flush_tlb();

  __asm__ volatile("mov %0, %%cr0" : : "r"(state->cr0) : "memory");

  mtrr_irq_restore(state->flags);
}

static bool_t mtrr_type_valid(uint8_t type) {
  switch (type) {
  case MTRR_UC:
  case MTRR_WT:
  case MTRR_WP:
  case MTRR_WB:
    return true;
  case MTRR_WC:
    return (mtrr_cap & MTRR_WC_SUPPORTED) != 0;
  default:
    return false;
  }
}

static bool_t mtrr_types_compatible(uint8_t a, uint8_t b) {
  return a == b || a == MTRR_UC || b == MTRR_UC ||
         (a == MTRR_WB && b == MTRR_WT) || (a == MTRR_WT && b == MTRR_WB);
}

int mtrr_init(void) {
  almond_cpuid_t cpu;
  uint32_t bits = 36;

  if (mtrr_ready)
    return 0;

  almond_cpuid_req(1, &cpu);

  if (!(cpu.edx & (1U << 5)) || !(cpu.edx & (1U << 12))) {
    message_send_message(" mtrr: el no supporto la MTRR tortilas tacos\n");
    return -1;
  }

  if (mtrr_extended_cpuid_eax(0x80000000U) >= 0x80000008U)
    bits = mtrr_extended_cpuid_eax(0x80000008U) & 0xFF;

  if (bits < 32 || bits > 52) {
    message_send_message(" mtrr: el no supporto la MTRR tortilas tacos: bits < 32 or bits > 52\n");
    return -1;
  }

  mtrr_cap = mtrr_rdmsr(MTRR_CAP);
  mtrr_count = (uint32_t)(mtrr_cap & 0xFF);
  mtrr_has_pge = (cpu.edx & (1U << 13)) != 0;
  mtrr_address_mask = ((1ULL << bits) - 1) & ~0xFFFULL;
  mtrr_ready = true;

  return 0;
}

int mtrr_set_region(uint32_t base, uint32_t size, uint8_t type) {
  struct mtrr_state state;
  uint64_t end = (uint64_t)base + size;
  uint32_t slot;

  if (mtrr_init() != 0)
    return -1;

  if (size < 0x1000 || (size & (size - 1)))
    return -1;

  if ((base & (size - 1)) || end > (1ULL << 32))
    return -1;

  if (!mtrr_type_valid(type))
    return -1;

  state.flags = mtrr_irq_save();
  slot = mtrr_count;

  for (uint32_t i = 0; i < mtrr_count; ++i) {
    uint64_t mask = mtrr_rdmsr(MTRR_MASK + i * 2);
    uint64_t region;
    uint64_t region_base;
    uint64_t region_size;

    if (!(mask & MTRR_VALID)) {
      if (slot == mtrr_count)
        slot = i;
      continue;
    }

    region = mtrr_rdmsr(MTRR_BASE + i * 2);
    region_base = region & mtrr_address_mask;
    region_size = ((~mask) & mtrr_address_mask) + 0x1000;

    if ((uint64_t)base < region_base + region_size && region_base < end &&
        !mtrr_types_compatible(type, (uint8_t)region)) {
      mtrr_irq_restore(state.flags);
      return -1;
    }
  }

  if (slot == mtrr_count) {
    mtrr_irq_restore(state.flags);
    return -1;
  }

  mtrr_begin(&state);

  mtrr_wrmsr(MTRR_BASE + slot * 2, (uint64_t)base | type);
  mtrr_wrmsr(MTRR_MASK + slot * 2,
             (~((uint64_t)size - 1) & mtrr_address_mask) | MTRR_VALID);

  mtrr_end(&state, state.def_type);
  return 0;
}

void mtrr_enable(bool_t enable) {
  struct mtrr_state state;
  uint64_t def_type;

  if (mtrr_init() != 0)
    return;

  state.flags = mtrr_irq_save();
  def_type = mtrr_rdmsr(MTRR_DEF_TYPE);

  if (((def_type & MTRR_ENABLED) != 0) == enable) {
    mtrr_irq_restore(state.flags);
    return;
  }

  mtrr_begin(&state);

  def_type = state.def_type & ~MTRR_ENABLED;
  if (enable)
    def_type |= MTRR_ENABLED;

  mtrr_end(&state, def_type);
}
