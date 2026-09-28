#ifndef PMM_H
#define PMM_H
#include "e820.h"
#define PMM_INVALID_PAGE 0xFFFFFFFFu
#ifndef PMM_BITMAP_USED
#define PMM_BITMAP_USED 1u
#endif
#ifndef PMM_BITMAP_FREE
#define PMM_BITMAP_FREE 0u
#endif
#ifndef PHYS_B_TO_PAGE
#define PHYS_B_TO_PAGE(n) ((n) >> 12)
#endif

// extern uint32_t pmm_size_mem // use this for getting max phys size (must be converted with the above PHYS_B_TO_PAGE
void pmm_bitmap_set_state(uint32_t offset, uint8_t state);
uint8_t pmm_bitmap_get_state(uint32_t offset);
uint32_t pmm_bitmap_alloc(void);
void pmm_bitmap_free(uint32_t offset);

void pmm_process_e820(e820_mmap_t *f, uint16_t mmap_count);
#endif
