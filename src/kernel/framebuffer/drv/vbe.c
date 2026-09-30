// vbe.c
// ----------------------
// Video Bios Extensions driver for Almond
// Uses standard fb0 interface.
#include "../../../boot/handoff_struct.h"
#include "../../helpers/string.h"
#include "../../kernel_types.h"
#include "../../messaging/messaging.h"
#include "../../mmu/kmalloc.h"
#include "../../mmu/vmm.h"
#include "../../mtrr.h"
#include "../fb.h"

#define VBE_VIRT_BASE 0xC0000000u

static volatile uint8_t *vbe_framebuffer;
static uint8_t *vbe_2buf;

static uint32_t vbe_mask_max(uint8_t width) {
  if (width >= 32) {
    return 0xFFFFFFFFu;
  }
  return (1u << width) - 1;
}

extern almond_handoff_t handoff_kerncopy;
bool_t vbe_discover(void) {
  if (handoff_kerncopy.vbe_framebuffer != 0)
    return true; // this would have been verified by bootloader.
  return false;
}

void vbe_update_fb(struct fb_surface *self) {
  memcpy((void *)vbe_framebuffer, (void *)vbe_2buf,
         handoff_kerncopy.vbe_pitch * handoff_kerncopy.vbe_height);
}

// also converts 32-bit RGBA to whatevers on the card
void vbe_put_pixel(uint16_t x, uint16_t y, uint32_t col,
                   struct fb_surface *self) {
  (void)self;
  uint32_t r = (col >> 24) & 0xFF;
  uint32_t g = (col >> 16) & 0xFF;
  uint32_t b = (col >> 8) & 0xFF;

  uint32_t r_max = vbe_mask_max(handoff_kerncopy.vbe_red_mask);
  uint32_t packed_r = ((r * r_max) / 255) << handoff_kerncopy.vbe_red_position;

  uint32_t g_max = vbe_mask_max(handoff_kerncopy.vbe_green_mask);
  uint32_t packed_g = ((g * g_max) / 255)
                      << handoff_kerncopy.vbe_green_position;

  uint32_t b_max = vbe_mask_max(handoff_kerncopy.vbe_blue_mask);
  uint32_t packed_b = ((b * b_max) / 255) << handoff_kerncopy.vbe_blue_position;
  uint32_t bytes_per_pixel = (handoff_kerncopy.vbe_bpp + 7) / 8;
  uint32_t offset;
  uint32_t packed_color = packed_r | packed_g | packed_b;

  if (x >= handoff_kerncopy.vbe_width || y >= handoff_kerncopy.vbe_height) {
    return;
  }

  offset = y * handoff_kerncopy.vbe_pitch + x * bytes_per_pixel;
  for (uint32_t byte = 0; byte < bytes_per_pixel; ++byte) {
    vbe_2buf[offset + byte] = (uint8_t)(packed_color >> (byte * 8));
  }
}

void vbe_init(fb_surface_t *target) {
  uint32_t bytes_per_pixel = (handoff_kerncopy.vbe_bpp + 7) / 8;
  char messagebuf[21];
  message_send_message(" vbe: framebuffer at: ");
  itoa_hex(handoff_kerncopy.vbe_framebuffer, messagebuf);
  message_send_message(messagebuf);
  message_send_message(" pitch=");
  itoa(handoff_kerncopy.vbe_pitch, messagebuf);
  message_send_message(messagebuf);
  message_send_message(" width=");
  itoa(handoff_kerncopy.vbe_width, messagebuf);
  message_send_message(messagebuf);
  message_send_message(" height=");
  itoa(handoff_kerncopy.vbe_height, messagebuf);
  message_send_message(messagebuf);
  message_send_message(" bpp=");
  itoa(handoff_kerncopy.vbe_bpp, messagebuf);
  message_send_message(messagebuf);
  message_send_message("\n");
  uint32_t physical_base = handoff_kerncopy.vbe_framebuffer & ~0xFFFu;
  uint32_t page_offset = handoff_kerncopy.vbe_framebuffer & 0xFFFu;
  uint32_t framebuffer_size =
      handoff_kerncopy.vbe_pitch * handoff_kerncopy.vbe_height;

  if (handoff_kerncopy.vbe_width == 0 || handoff_kerncopy.vbe_height == 0 ||
      (handoff_kerncopy.vbe_bpp != 15 && handoff_kerncopy.vbe_bpp != 16 &&
       handoff_kerncopy.vbe_bpp != 24 && handoff_kerncopy.vbe_bpp != 32) ||
      handoff_kerncopy.vbe_pitch <
          handoff_kerncopy.vbe_width * bytes_per_pixel ||
      framebuffer_size == 0 || framebuffer_size > 0x04000000u ||
      handoff_kerncopy.vbe_framebuffer > 0xFFFFFFFFu - framebuffer_size) {
    return;
  }
  uint32_t offset = 0;
  if (page_offset == 0 && (physical_base & 0x003FFFFFu) == 0) {
    uint32_t mapped_size = (framebuffer_size + 0x003FFFFFu) & 0xFFC00000u;
    while (offset < mapped_size) {
      vmm_map_phys_4mb(vmm_current_pagemap(), physical_base + offset,
                       VBE_VIRT_BASE + offset,
                       VMM_PAGE_PRESENT_RW | VMM_PAGE_WRITE_THROUGH |
                           VMM_PAGE_CACHE_DISABLE);
      offset += 0x00400000u;
    }
  }

  uint64_t framebuffer_end = (uint64_t)framebuffer_size + page_offset;
  uint32_t offset_asfas = page_offset;
  bool_t configure_mtrr = mtrr_init() == 0;
  if (configure_mtrr == false) {
    message_send_message(" vbe: could not init mtrr\n");
  }

  while ((uint64_t)offset_asfas < framebuffer_end) {
    uint32_t physical = physical_base + offset_asfas;
    uint32_t remaining = (uint32_t)(framebuffer_end - offset_asfas);
    uint32_t region_size = VMM_PAGE_SIZE;

    while (region_size <= (remaining >> 1) &&
           (physical & ((region_size << 1) - 1)) == 0) {
      region_size <<= 1;
    }

    if (configure_mtrr &&
        mtrr_set_region(physical,
                        (region_size--, region_size |= region_size >> 1,
                         region_size |= region_size >> 2,
                         region_size |= region_size >> 4,
                         region_size |= region_size >> 8,
                         region_size |= region_size >> 16, region_size + 1),
                        MTRR_TYPE_WC) != 0) {

      message_send_message(" vbe: could not set region to WC\n");
      configure_mtrr = false;
    }

    for (uint32_t mapped = 0;
         mapped < region_size && (uint64_t)offset_asfas < framebuffer_end;
         mapped += VMM_PAGE_SIZE, offset_asfas += VMM_PAGE_SIZE) {
      vmm_map_phys_to_virt_flags(
          vmm_current_pagemap(), physical_base + offset_asfas,
          VBE_VIRT_BASE + offset_asfas, VMM_PAGE_PRESENT_RW);
    }
  }
  if (configure_mtrr)
    mtrr_enable(true);
  else
    message_send_message(" vbe: failed to configure MTRR for framebuffer\n");
  vbe_framebuffer = (volatile uint8_t *)(VBE_VIRT_BASE + page_offset);
  vbe_2buf = kmalloc(handoff_kerncopy.vbe_pitch * handoff_kerncopy.vbe_height);
  if (vbe_2buf == 0) {
    message_send_message(" vbe: failed to allocate 2nd framebuffer\n");
    vbe_2buf = (uint8_t *)vbe_framebuffer;
  }
  message_send_message(" vbe: DBuf @ ");
  itoa_hex((uint32_t)vbe_framebuffer, messagebuf);
  message_send_message(messagebuf);
  message_send_message("\n");
  target->height = handoff_kerncopy.vbe_height;
  target->width = handoff_kerncopy.vbe_width;
  target->write_pixel = vbe_put_pixel;
  target->update_fb = vbe_update_fb;
}
