#include "kmalloc.h"
#include "pmm.h"
#include "vmm.h"
#include "../helpers/string.h"
#include "../messaging/messaging.h"

#define KMALLOC_MAGIC 0x4B4D414Cu
#define KMALLOC_LARGE_CLASS 0xFFFFu
#define KMALLOC_CLASS_COUNT 8u

typedef struct kmalloc_header {
  uint32_t magic;
  uint16_t class_index;
  uint16_t reserved;
  uint32_t page_count;
  struct kmalloc_header *next;
} kmalloc_header_t;

typedef struct kmalloc_slab {
  uint32_t magic;
  uint16_t class_index;
  uint16_t capacity;
  uint16_t used;
  uint16_t reserved;
  struct kmalloc_slab *next;
  kmalloc_header_t *free_list;
} kmalloc_slab_t;

static const uint16_t kmalloc_sizes[KMALLOC_CLASS_COUNT] = {
  16, 32, 64, 128, 256, 512, 1024, 2048
};
static kmalloc_slab_t *kmalloc_slabs[KMALLOC_CLASS_COUNT];
static uint8_t kmalloc_ready;

static uint32_t kmalloc_align(uint32_t value) {
  return (value + 7u) & ~7u;
}

static uint32_t kmalloc_find_pages(uint32_t count) {
  uint32_t total_pages = (VMM_KERNEL_HEAP_END - VMM_KERNEL_HEAP_BASE) / VMM_PAGE_SIZE;
  uint32_t run_length = 0;
  vmm_pagemap_t pagemap = vmm_current_pagemap();

  if (count == 0 || count > total_pages) {
    return 0;
  }
  for (uint32_t page = 0; page < total_pages; ++page) {
    uint32_t address = VMM_KERNEL_HEAP_BASE + page * VMM_PAGE_SIZE;
    if ((vmm_get_page_state(pagemap, address) & 1u) == 0) {
      ++run_length;
      if (run_length == count) {
        return address - (count - 1) * VMM_PAGE_SIZE;
      }
    } else {
      run_length = 0;
    }
  }
  return 0;
}

static void *kmalloc_map_pages(uint32_t count) {
  uint32_t virtual_address = kmalloc_find_pages(count);
  uint32_t page;
  vmm_pagemap_t pagemap = vmm_current_pagemap();

  if (virtual_address == 0) {
    message_send_message(" kmalloc: no virtual run\n");
    return 0;
  }
  for (page = 0; page < count; ++page) {
    uint32_t physical_page = pmm_bitmap_alloc();
    if (physical_page == PMM_INVALID_PAGE) {
      char progress[16];
      itoa((int)page, progress);
      message_send_message(" kmalloc: physical pages exhausted after ");
      message_send_message(progress);
      message_send_message(" pages\n");
      while (page != 0) {
        uint32_t state;
        --page;
        state = vmm_get_page_state(pagemap, virtual_address + page * VMM_PAGE_SIZE);
        vmm_unmap_virt(pagemap, virtual_address + page * VMM_PAGE_SIZE);
        pmm_bitmap_free(state >> 12);
      }
      return 0;
    }
    vmm_map_phys_to_virt(pagemap, physical_page << 12, virtual_address + page * VMM_PAGE_SIZE);
    if ((vmm_get_page_state(pagemap, virtual_address + page * VMM_PAGE_SIZE) & 1u) == 0) {
      message_send_message(" kmalloc: page mapping failed\n");
      pmm_bitmap_free(physical_page);
      while (page != 0) {
        uint32_t state;
        --page;
        state = vmm_get_page_state(pagemap, virtual_address + page * VMM_PAGE_SIZE);
        vmm_unmap_virt(pagemap, virtual_address + page * VMM_PAGE_SIZE);
        pmm_bitmap_free(state >> 12);
      }
      return 0;
    }
  }
  return (void *)virtual_address;
}

static void kmalloc_unmap_pages(void *address, uint32_t count) {
  uint32_t page;
  vmm_pagemap_t pagemap = vmm_current_pagemap();

  for (page = 0; page < count; ++page) {
    uint32_t virtual_address = (uint32_t)address + page * VMM_PAGE_SIZE;
    uint32_t state = vmm_get_page_state(pagemap, virtual_address);
    vmm_unmap_virt(pagemap, virtual_address);
    if ((state & 1u) != 0) {
      pmm_bitmap_free(state >> 12);
    }
  }
}

static kmalloc_slab_t *kmalloc_new_slab(uint16_t class_index) {
  uint32_t stride = kmalloc_align(sizeof(kmalloc_header_t) + kmalloc_sizes[class_index]);
  uint32_t offset = kmalloc_align(sizeof(kmalloc_slab_t));
  kmalloc_slab_t *slab = kmalloc_map_pages(1);
  kmalloc_header_t *header;

  if (slab == 0 || offset + stride > VMM_PAGE_SIZE) {
    return 0;
  }
  slab->magic = KMALLOC_MAGIC;
  slab->class_index = class_index;
  slab->capacity = (VMM_PAGE_SIZE - offset) / stride;
  slab->used = 0;
  slab->free_list = 0;
  for (uint32_t index = 0; index < slab->capacity; ++index) {
    header = (kmalloc_header_t *)((uint32_t)slab + offset + index * stride);
    header->magic = KMALLOC_MAGIC;
    header->class_index = class_index;
    header->page_count = 0;
    header->next = slab->free_list;
    slab->free_list = header;
  }
  slab->next = kmalloc_slabs[class_index];
  kmalloc_slabs[class_index] = slab;
  return slab;
}

void kmalloc_init(void) {
  for (uint32_t index = 0; index < KMALLOC_CLASS_COUNT; ++index) {
    kmalloc_slabs[index] = 0;
  }
  kmalloc_ready = 1;
}

void *kmalloc(uint32_t size) {
  uint16_t class_index;
  kmalloc_slab_t *slab;
  kmalloc_header_t *header;

  if (size == 0 || kmalloc_ready == 0) {
    return 0;
  }
  for (class_index = 0; class_index < KMALLOC_CLASS_COUNT; ++class_index) {
    if (size <= kmalloc_sizes[class_index]) {
      break;
    }
  }
  if (class_index == KMALLOC_CLASS_COUNT) {
    uint32_t overhead = sizeof(kmalloc_header_t) + VMM_PAGE_SIZE - 1;
    if (size > 0xFFFFFFFFu - overhead) {
      return 0;
    }
    uint32_t pages = (size + overhead) / VMM_PAGE_SIZE;
    if (pages > (VMM_KERNEL_HEAP_END - VMM_KERNEL_HEAP_BASE) / VMM_PAGE_SIZE) {
      return 0;
    }
    header = kmalloc_map_pages(pages);
    if (header == 0) {
      return 0;
    }
    header->magic = KMALLOC_MAGIC;
    header->class_index = KMALLOC_LARGE_CLASS;
    header->page_count = pages;
    return header + 1;
  }

  for (slab = kmalloc_slabs[class_index]; slab != 0; slab = slab->next) {
    if (slab->free_list != 0) {
      break;
    }
  }
  if (slab == 0) {
    slab = kmalloc_new_slab(class_index);
    if (slab == 0) {
      return 0;
    }
  }
  header = slab->free_list;
  slab->free_list = header->next;
  header->magic = KMALLOC_MAGIC;
  ++slab->used;
  return header + 1;
}

void *kmalloc_aligned(uint32_t size, uint32_t align) {
  // If alignment is 0 or 1, standard kmalloc is sufficient
  if (align <= 1) {
    return kmalloc(size);
  }

  uint32_t total_size = size + align + sizeof(void *);

  void *raw_ptr = kmalloc(total_size);
  if (raw_ptr == 0) {
    return 0;
  }

  // Calculate the raw address plus space reserved for storing the original pointer
  uint32_t raw_addr = (uint32_t)(uintptr_t)raw_ptr + sizeof(void *);

  // Align the address upward to the nearest multiple of 'align'
  uint32_t aligned_addr = (raw_addr + align - 1) & ~(align - 1);

  // Store the original raw pointer right before the aligned address
  void **stored_ptr_slot = (void **)(uintptr_t)(aligned_addr - sizeof(void *));
  *stored_ptr_slot = raw_ptr;

  return (void *)aligned_addr;
}

void kfree(void *ptr) {
  kmalloc_header_t *header;

  if (ptr == 0 || kmalloc_ready == 0) {
    return;
  }
  header = ((kmalloc_header_t *)ptr) - 1;
  if (header->magic != KMALLOC_MAGIC) {
    return;
  }
  if (header->class_index == KMALLOC_LARGE_CLASS) {
    uint32_t pages = header->page_count;
    header->magic = 0;
    kmalloc_unmap_pages(header, pages);
    return;
  }
  if (header->class_index < KMALLOC_CLASS_COUNT) {
    kmalloc_slab_t *slab = (kmalloc_slab_t *)((uint32_t)header & ~(VMM_PAGE_SIZE - 1u));
    if (slab->magic != KMALLOC_MAGIC || slab->used == 0) {
      return;
    }
    header->magic = 0;
    header->next = slab->free_list;
    slab->free_list = header;
    --slab->used;
    if (slab->used == 0) {
      kmalloc_slab_t **link = &kmalloc_slabs[slab->class_index];
      while (*link != slab) {
        link = &(*link)->next;
      }
      *link = slab->next;
      slab->magic = 0;
      kmalloc_unmap_pages(slab, 1);
    }
  }
}

void kfree_aligned(void *ptr) {
  if (ptr == 0) {
    return;
  }
  // Retrieve the original raw pointer stored right before the aligned address
  void *raw_ptr = *((void **)((uint32_t)(uintptr_t)ptr - sizeof(void *)));

  // Pass the original pointer to your base kfree implementation
  kfree(raw_ptr);
}

// get phys address.
// Returns 0 if not mapped or null
uint32_t kmalloc_virt_to_phys(uint32_t virtual_address) {
  vmm_pagemap_t pagemap = vmm_current_pagemap();
  uint32_t state = vmm_get_page_state(pagemap, virtual_address);

  if ((state & 1u) == 0) {
    return 0;
  }
  return (state & ~(VMM_PAGE_SIZE - 1u)) | (virtual_address & (VMM_PAGE_SIZE - 1u));
}
