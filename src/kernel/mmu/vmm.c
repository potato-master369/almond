// vmm.c
// -----------------------------------
// Handles virtual memory management in paging
#include "vmm.h" 
#include "pmm.h"

#define VMM_PRESENT_RW 0x003u
#define VMM_RECURSIVE_PD 0xFFFFF000u
#define VMM_RECURSIVE_PT 0xFFC00000u
#define VMM_KERNEL_VIRT_BASE 0xC8000000u
#define VMM_KERNEL_PHYS_BASE 0x00100000u

static void vmm_reload_cr3(void) {
  uint32_t cr3;
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

static void vmm_invalidate(uint32_t virt_offset) {
  __asm__ volatile("invlpg (%0)" : : "r"(virt_offset) : "memory");
}

vmm_pagemap_t vmm_current_pagemap(void) {
  return (vmm_pagemap_t)VMM_RECURSIVE_PD;
}

void vmm_init(void) {
  uint32_t cr3;
  vmm_pagemap_t initial_pagemap;

  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  initial_pagemap = (vmm_pagemap_t)(VMM_KERNEL_VIRT_BASE + cr3 - VMM_KERNEL_PHYS_BASE);
  initial_pagemap[1023].raw = (cr3 & 0xFFFFF000u) | VMM_PRESENT_RW;
  for (uint32_t i = 0; i < 8; ++i) {
    initial_pagemap[i].raw = 0;
  }
  vmm_reload_cr3();
}

uint32_t vmm_get_page_state(vmm_pagemap_t f, uint32_t virt_offset) {
  uint32_t directory_index = virt_offset >> 22;
  uint32_t table_index = (virt_offset >> 12) & 0x3FFu;
  vmm_pt_t *page_table;

  if ((f[directory_index].raw & 1u) == 0) {
    return 0;
  }
  if (f[directory_index].bits.page_size != 0) {
    return f[directory_index].raw;
  }

  page_table = (vmm_pt_t *)(VMM_RECURSIVE_PT + directory_index * VMM_PAGE_SIZE);
  return page_table[table_index].raw;
}

void vmm_map_phys_to_virt(vmm_pagemap_t f, uint32_t phys_offset, uint32_t virt_offset) {
  vmm_map_phys_to_virt_flags(f, phys_offset, virt_offset, VMM_PAGE_PRESENT_RW);
}

void vmm_map_phys_to_virt_flags(vmm_pagemap_t f, uint32_t phys_offset,
                                uint32_t virt_offset, uint32_t flags) {
  uint32_t directory_index = virt_offset >> 22;
  uint32_t table_index = (virt_offset >> 12) & 0x3FFu;
  vmm_pt_t *page_table;

  flags |= VMM_PRESENT_RW;
  phys_offset &= 0xFFFFF000u;
  virt_offset &= 0xFFFFF000u;
  if ((f[directory_index].raw & 1u) == 0) {
    uint32_t table_page = pmm_bitmap_alloc();
    if (table_page == PMM_INVALID_PAGE) {
      return;
    }
    f[directory_index].raw = (table_page << 12) | VMM_PRESENT_RW |
                 (flags & (VMM_PAGE_WRITE_THROUGH | VMM_PAGE_CACHE_DISABLE));
    page_table = (vmm_pt_t *)(VMM_RECURSIVE_PT + directory_index * VMM_PAGE_SIZE);
    vmm_invalidate((uint32_t)page_table);
    for (uint32_t i = 0; i < 1024; ++i) {
      page_table[i].raw = 0;
    }
  } else {
    page_table = (vmm_pt_t *)(VMM_RECURSIVE_PT + directory_index * VMM_PAGE_SIZE);
  }

  page_table[table_index].raw = phys_offset | flags;
  vmm_invalidate(virt_offset);
}

void vmm_map_phys_4mb(vmm_pagemap_t f, uint32_t phys_offset,
                      uint32_t virt_offset, uint32_t flags) {
  uint32_t cr4;
  uint32_t directory_index;

  if ((phys_offset & 0x003FFFFFu) != 0 || (virt_offset & 0x003FFFFFu) != 0) {
    return;
  }
  __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
  cr4 |= 1u << 4;
  __asm__ volatile("mov %0, %%cr4" : : "r"(cr4) : "memory");
  directory_index = virt_offset >> 22;
  f[directory_index].raw = (phys_offset & 0xFFC00000u) |
                           (flags & 0x0000001Fu) | 0x083u;
  vmm_reload_cr3();
}

void vmm_unmap_virt(vmm_pagemap_t f, uint32_t virt_offset) {
  uint32_t directory_index = virt_offset >> 22;
  uint32_t table_index = (virt_offset >> 12) & 0x3FFu;
  vmm_pt_t *page_table;
  uint32_t i;

  virt_offset &= 0xFFFFF000u;
  if ((f[directory_index].raw & 1u) == 0 || f[directory_index].bits.page_size != 0) {
    return;
  }

  page_table = (vmm_pt_t *)(VMM_RECURSIVE_PT + directory_index * VMM_PAGE_SIZE);
  page_table[table_index].raw = 0;
  vmm_invalidate(virt_offset);
  for (i = 0; i < 1024; ++i) {
    if ((page_table[i].raw & 1u) != 0) {
      return;
    }
  }

  pmm_bitmap_free(f[directory_index].bits.pt_frame);
  f[directory_index].raw = 0;
  vmm_reload_cr3();
}
