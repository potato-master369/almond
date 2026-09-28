// pmm.c
// -------------------------------------
// Physical memory manager

#include "pmm.h"
#include "../helpers/string.h"
#include "../kernel_types.h"
#include "../messaging/messaging.h"
#include "e820.h"
#include <iso646.h>
uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem) {
    uint64_t quot = 0, qbit = 1;

    if (den == 0) return 0; // Handle divide-by-zero

    while ((int64_t)den >= 0 && den < num) {
        den <<= 1;
        qbit <<= 1;
    }

    while (qbit != 0) {
        if (num >= den) {
            num -= den;
            quot |= qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }

    if (rem) *rem = num;
    return quot;
}

extern char _kernel_virt_end;
uint32_t pmm_size_mem;
char *pmm_bitmap;
uint32_t pmm_last_alloc = 0;

// helpers to deal with the bitmap
#define PMM_BITMAP_USED 1u
#define PMM_BITMAP_FREE 0u
#define PHYS_B_TO_PAGE(n) ((n) >> 12) // equivalent to n / 4096 but faster

// state is a uint8_t. Only the LSB has a fuck given about it.
void pmm_bitmap_set_state(uint32_t offset, uint8_t state) {
  pmm_bitmap[offset / 8] = (pmm_bitmap[offset / 8] & ~(1 << (offset % 8))) |
                           ((state ? 1 : 0) << (offset % 8));
}

uint8_t pmm_bitmap_get_state(uint32_t offset) {
  return ((pmm_bitmap[offset / 8] >> (offset % 8)) & 1);
}

// allocate page in bitmap
uint32_t pmm_bitmap_alloc(void) {
  if (pmm_bitmap == 0) {
    pmm_bitmap = &_kernel_virt_end;
  }
  uint32_t page_count = PHYS_B_TO_PAGE(pmm_size_mem);
  uint32_t start;

  if (page_count == 0) {
    return PMM_INVALID_PAGE;
  }

  start = pmm_last_alloc < page_count ? pmm_last_alloc : 0;
  for (uint32_t scanned = 0; scanned < page_count; ++scanned) {
    uint32_t page = start + scanned;
    if (page >= page_count) {
      page -= page_count;
    }
    if (pmm_bitmap_get_state(page) == PMM_BITMAP_FREE) {
      pmm_bitmap_set_state(page, PMM_BITMAP_USED);
      pmm_last_alloc = page + 1;
      if (pmm_last_alloc == page_count) {
        pmm_last_alloc = 0;
      }
      return page;
    }
  }

  return PMM_INVALID_PAGE;
}

void pmm_bitmap_free(uint32_t offset) {
  if (offset < PHYS_B_TO_PAGE(pmm_size_mem)) {
    pmm_bitmap_set_state(offset, PMM_BITMAP_FREE);
  }
}

static void pmm_set_range(uint64_t base, uint64_t length, uint8_t state,
                          bool_t reserve_partial_pages) {
  uint64_t end;
  uint64_t start_page;
  uint64_t end_page;

  if (length == 0 || base >= pmm_size_mem) {
    return;
  }
  end = base + length;
  if (end < base || end > pmm_size_mem) {
    end = pmm_size_mem;
  }
  if (reserve_partial_pages) {
    start_page = base >> 12;
    end_page = (end + 0xFFFu) >> 12;
  } else {
    start_page = (base + 0xFFFu) >> 12;
    end_page = end >> 12;
  }
  if (end_page > PHYS_B_TO_PAGE(pmm_size_mem)) {
    end_page = PHYS_B_TO_PAGE(pmm_size_mem);
  }
  for (uint64_t page = start_page; page < end_page; ++page) {
    pmm_bitmap_set_state((uint32_t)page, state);
  }
}

void swap_e820_entries(e820_mmap_entry_t *a, e820_mmap_entry_t *b) {
    e820_mmap_entry_t temp = *a;
    *a = *b;
    *b = temp;
}

void pmm_process_e820(e820_mmap_t *f, uint16_t mmap_count) {
  pmm_bitmap = &_kernel_virt_end;
  pmm_last_alloc = 0;

  char reportbuf[21];
  int64_t total_mem = 0;
  int64_t total_mem_all = 0;
  uint64_t max_highest_addr = 0;

  for (uint16_t i = 0; i < mmap_count - 1; ++i) {
    for (uint16_t j = 0; j < mmap_count - i - 1; ++j) {
      if (f->entries[j].base_addr > f->entries[j+1].base_addr) {
        swap_e820_entries(&f->entries[j], &f->entries[j+1]);
      }
    }
  }

  for (uint16_t i = 0; i < mmap_count; ++i) {
    uint64_t end = f->entries[i].base_addr + f->entries[i].region_len;
    if (end > max_highest_addr) {
      max_highest_addr = end;
    }
  }

  pmm_size_mem = max_highest_addr > 0xFFFFFFFFull
                     ? 0xFFFFFFFFu
                     : (uint32_t)max_highest_addr;
  for (uint32_t i = 0; i < (PHYS_B_TO_PAGE(pmm_size_mem) + 7) / 8; ++i) {
    pmm_bitmap[i] = 0xFF;
  }

  for (uint16_t i = 0; i < mmap_count; ++i) {
    uint64_t length = f->entries[i].region_len;
    utoa64(f->entries[i].base_addr, reportbuf);
    message_send_message(reportbuf);
    message_send_message(" : ");

    utoa64(length, reportbuf);
    message_send_message(reportbuf);

    switch (f->entries[i].region_type) {
    case 1:
    case 7:
      message_send_message(" US\n");

      total_mem += length;
      break;

    case 2: message_send_message(" RS\n"); break;
    case 3: message_send_message(" AR\n"); break;
    case 4: message_send_message(" NS\n"); break;
    case 5: message_send_message(" BAD!\n"); break;
    default:
      itoa(f->entries[i].region_type, reportbuf);
      message_send_message(" INVALID TYPE: ");
      message_send_message(reportbuf);
      message_send_message("\n");
      break;
    }

    total_mem_all += length;
  }

  for (uint16_t i = 0; i < mmap_count; ++i) {
    uint32_t type = f->entries[i].region_type;
    if (type == 1 || type == 7) {
      pmm_set_range(f->entries[i].base_addr, f->entries[i].region_len,
                    PMM_BITMAP_FREE, FALSE);
    }
  }
  for (uint16_t i = 0; i < mmap_count; ++i) {
    uint32_t type = f->entries[i].region_type;
    if (type >= 2 && type <= 5) {
      pmm_set_range(f->entries[i].base_addr, f->entries[i].region_len,
                    PMM_BITMAP_USED, TRUE);
    }
  }

  utoa64(total_mem_all / 1024, reportbuf);
  message_send_message(reportbuf);
  message_send_message("KiB, ");
  utoa64(total_mem / 1024, reportbuf);
  message_send_message(reportbuf);
  message_send_message(" OK\n");

  uint32_t bitmap_end = ((uint32_t)&_kernel_virt_end - 0xC8000000) + 0x00100000;
  bitmap_end += (PHYS_B_TO_PAGE(pmm_size_mem) + 7) / 8;
  bitmap_end = (bitmap_end + 0xFFF) & ~0xFFF;
  for (uint32_t i = 0; i < bitmap_end; i += 4096) {
    pmm_bitmap_set_state(PHYS_B_TO_PAGE(i), PMM_BITMAP_USED);
  }
  for (uint32_t addr = 0x01F00000u; addr < 0x02000000u; addr += 4096) {
    pmm_bitmap_set_state(PHYS_B_TO_PAGE(addr), PMM_BITMAP_USED);
  }

  uint32_t free_pages = 0;
  for (uint32_t page = 0; page < PHYS_B_TO_PAGE(pmm_size_mem); ++page) {
    if (pmm_bitmap_get_state(page) == PMM_BITMAP_FREE) {
      ++free_pages;
    }
  }
  itoa((int)free_pages, reportbuf);
  message_send_message("PMM free pages: ");
  message_send_message(reportbuf);
  message_send_message("\n");
}

