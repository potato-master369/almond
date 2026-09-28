#ifndef VMM_H
#define VMM_H
#include "../kernel_types.h"
typedef union {
  struct __attribute__((packed)) {
    uint32_t present : 1;   // 1 = Page is in memory, 0 = Not present
    uint32_t rw : 1;        // 0 = Read-only, 1 = Read/Write
    uint32_t user : 1;      // 0 = Supervisor only, 1 = User accessible
    uint32_t pwt : 1;       // Page-level write-through
    uint32_t pcd : 1;       // Page-level cache disable
    uint32_t accessed : 1;  // Has the page been read/written?
    uint32_t dirty : 1;     // Has the page been written to?
    uint32_t pat : 1;       // Page attribute table index
    uint32_t global : 1;    // Global page (prevents TLB flush)
    uint32_t available : 3; // Available for OS use
    uint32_t frame : 20;    // Top 20 bits of physical frame address
  } bits;
  uint32_t raw;
} vmm_pt_t;

typedef union {
  struct {
    uint32_t present : 1;   // 1 = Page table is present
    uint32_t rw : 1;        // 0 = Read-only, 1 = Read/Write
    uint32_t user : 1;      // 0 = Supervisor only, 1 = User accessible
    uint32_t pwt : 1;       // Write-through
    uint32_t pcd : 1;       // Cache disable
    uint32_t accessed : 1;  // Has the page table been accessed?
    uint32_t reserved : 1;  // Reserved (0)
    uint32_t page_size : 1; // 0 = 4KB page, 1 = 4MB large page
    uint32_t global : 1;    // Ignored for PDEs
    uint32_t available : 3; // Available for OS use
    uint32_t pt_frame : 20; // Top 20 bits of Page Table physical address
  } bits;
  uint32_t raw;
} vmm_pd_t;

typedef vmm_pd_t *vmm_pagemap_t;
#define VMM_PAGE_SIZE 4096u
#define VMM_PAGE_PRESENT_RW 0x003u
#define VMM_PAGE_WRITE_THROUGH 0x008u
#define VMM_PAGE_CACHE_DISABLE 0x010u
#define VMM_KERNEL_HEAP_BASE 0xD0000000u
#define VMM_KERNEL_HEAP_END 0xEFC00000u

void vmm_init(void);
vmm_pagemap_t vmm_current_pagemap(void);
void vmm_map_phys_to_virt(vmm_pagemap_t f, uint32_t phys_offset, uint32_t virt_offset);
void vmm_map_phys_to_virt_flags(vmm_pagemap_t f, uint32_t phys_offset,
                                uint32_t virt_offset, uint32_t flags);
void vmm_map_phys_4mb(vmm_pagemap_t f, uint32_t phys_offset,
                      uint32_t virt_offset, uint32_t flags);
void vmm_unmap_virt(vmm_pagemap_t f, uint32_t virt_offset);
uint32_t vmm_get_page_state(vmm_pagemap_t f, uint32_t virt_offset);
#endif
