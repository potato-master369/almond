// vbe.c
// ----------------------
// Video Bios Extensions driver for Almond
// Uses standard fb0 interface.
#include "../boot/handoff_struct.h"
#include "../fb.h"
#include "helpers/string.h"
#include "kernel_types.h"
#include "messaging/messaging.h"
#include "mmu/kmalloc.h"
#include "mmu/vmm.h"
#include "mtrr.h"

#define VBE_VIRT_BASE 0xC0000000u
#define VBE_4MB 0x00400000u

extern almond_handoff_t handoff_kerncopy;

static volatile uint8_t *vbe_framebuffer;
static uint8_t *vbe_2buf;
static bool_t vbe_direct = false;

static struct {
  uint8_t rpos, gpos, bpos;
  uint8_t rlose, glose, blose;
  uint8_t bpp_bytes;
  uint32_t pitch;
  uint32_t width;
  uint32_t height;
  uint32_t size;
} fmt;

static uint32_t dirty_x0 = 0xFFFFFFFFu, dirty_y0 = 0xFFFFFFFFu;
static uint32_t dirty_x1 = 0, dirty_y1 = 0;

static inline uint32_t irq_save(void) {
  uint32_t flags;
  __asm__ volatile("pushfl; popl %0; cli" : "=r"(flags)::"memory");
  return flags;
}

static inline void irq_restore(uint32_t flags) {
  __asm__ volatile("pushl %0; popfl" ::"r"(flags) : "memory", "cc");
}

static inline void dirty_reset(void) {
  dirty_x0 = 0xFFFFFFFFu;
  dirty_y0 = 0xFFFFFFFFu;
  dirty_x1 = 0;
  dirty_y1 = 0;
}

static inline void vbe_mark_dirty(uint32_t x0, uint32_t y0, uint32_t x1,
                                  uint32_t y1) {
  if (x0 < dirty_x0)
    dirty_x0 = x0;
  if (y0 < dirty_y0)
    dirty_y0 = y0;
  if (x1 > dirty_x1)
    dirty_x1 = x1;
  if (y1 > dirty_y1)
    dirty_y1 = y1;
}

static inline uint32_t vbe_pack(uint32_t col) {
  uint32_t r = (col >> 24) & 0xFF;
  uint32_t g = (col >> 16) & 0xFF;
  uint32_t b = (col >> 8) & 0xFF;
  return ((r >> fmt.rlose) << fmt.rpos) | ((g >> fmt.glose) << fmt.gpos) |
         ((b >> fmt.blose) << fmt.bpos);
}

static inline void vbe_store(uint8_t *p, uint32_t packed) {
  switch (fmt.bpp_bytes) {
  case 4:
    *(uint32_t *)p = packed;
    break;
  case 2:
    *(uint16_t *)p = (uint16_t)packed;
    break;
  case 3:
    p[0] = (uint8_t)packed;
    p[1] = (uint8_t)(packed >> 8);
    p[2] = (uint8_t)(packed >> 16);
    break;
  default:
    break; // do nothing (:
  }
}

bool_t vbe_discover(void) {
  if (handoff_kerncopy.vbe_framebuffer != 0)
    return true; // ABL should have handled setting the fb to 0.
  return false;
}

void vbe_update_fb(struct fb_surface *self) {
  (void)self;

  if (vbe_direct)
    return;

  uint32_t flags = irq_save();
  uint32_t x0 = dirty_x0, y0 = dirty_y0, x1 = dirty_x1, y1 = dirty_y1;
  dirty_reset();
  irq_restore(flags);

  if (x0 >= x1 || y0 >= y1)
    return;

  if (x1 > fmt.width)
    x1 = fmt.width;
  if (y1 > fmt.height)
    y1 = fmt.height;
  if (x0 >= x1 || y0 >= y1)
    return;

  if (x0 == 0 && x1 == fmt.width) {
    uint32_t start = y0 * fmt.pitch;
    memcpy((void *)(vbe_framebuffer + start), (void *)(vbe_2buf + start),
           (y1 - y0) * fmt.pitch);
    return;
  }

  uint32_t row_off = x0 * fmt.bpp_bytes;
  uint32_t row_len = (x1 - x0) * fmt.bpp_bytes;
  for (uint32_t y = y0; y < y1; ++y) {
    uint32_t off = y * fmt.pitch + row_off;
    memcpy((void *)(vbe_framebuffer + off), (void *)(vbe_2buf + off), row_len);
  }
}

void vbe_put_pixel(uint32_t x, uint32_t y, uint32_t col,
                   struct fb_surface *self) {
  (void)self;

  if (x >= fmt.width || y >= fmt.height)
    return;

  uint32_t off = y * fmt.pitch + x * fmt.bpp_bytes;
  vbe_store(vbe_2buf + off, vbe_pack(col));
  vbe_mark_dirty(x, y, (uint32_t)x + 1, (uint32_t)y + 1);
}

void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col,
                   struct fb_surface *self) {
  (void)self;
  if (x >= fmt.width || y >= fmt.height || w == 0 || h == 0)
    return;
  if (w > fmt.width - x)
    w = fmt.width - x;
  if (h > fmt.height - y)
    h = fmt.height - y;

  uint32_t packed = vbe_pack(col);

  for (uint32_t row = 0; row < h; ++row) {
    uint8_t *p = vbe_2buf + (y + row) * fmt.pitch + x * fmt.bpp_bytes;

    if (fmt.bpp_bytes == 4) {
      uint32_t n = w;
      __asm__ volatile("rep stosl" : "+D"(p), "+c"(n) : "a"(packed) : "memory");
    } else if (fmt.bpp_bytes == 2) {
      uint16_t *p16 = (uint16_t *)p;
      for (uint32_t i = 0; i < w; ++i)
        p16[i] = (uint16_t)packed;
    } else {
      for (uint32_t i = 0; i < w; ++i, p += 3)
        vbe_store(p, packed);
    }
  }

  vbe_mark_dirty(x, y, x + w, y + h);
}

void vbe_clear(uint32_t col, struct fb_surface *self) {
  (void)self;
  vbe_fill_rect(0, 0, fmt.width, fmt.height, col, NULL);
}

void vbe_blit_rgba(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                   const uint32_t *src, uint32_t src_stride,
                   struct fb_surface *self) {
  (void)self;
  if (x >= fmt.width || y >= fmt.height || w == 0 || h == 0 || src == 0)
    return;

  uint32_t cw = w, ch = h;
  if (cw > fmt.width - x)
    cw = fmt.width - x;
  if (ch > fmt.height - y)
    ch = fmt.height - y;

  for (uint32_t row = 0; row < ch; ++row) {
    const uint32_t *s = src + row * src_stride;
    uint8_t *d = vbe_2buf + (y + row) * fmt.pitch + x * fmt.bpp_bytes;

    if (fmt.bpp_bytes == 4) {
      uint32_t *d32 = (uint32_t *)d;
      for (uint32_t i = 0; i < cw; ++i)
        d32[i] = vbe_pack(s[i]);
    } else {
      for (uint32_t i = 0; i < cw; ++i, d += fmt.bpp_bytes)
        vbe_store(d, vbe_pack(s[i]));
    }
  }

  vbe_mark_dirty(x, y, x + cw, y + ch);
}

void vbe_scroll_up(uint32_t lines, uint32_t fill_col, struct fb_surface *self) {
  (void)self;
  if (lines == 0)
    return;
  if (lines >= fmt.height) {
    vbe_clear(fill_col, NULL);
    return;
  }
  memmove(vbe_2buf, vbe_2buf + lines * fmt.pitch,
          (fmt.height - lines) * fmt.pitch);
  vbe_fill_rect(0, fmt.height - lines, fmt.width, lines, fill_col, NULL);
  vbe_mark_dirty(0, 0, fmt.width, fmt.height);
}

// a function to make life a bit easier
static void vbe_log_u32(char *label, uint32_t v, bool_t hex) {
  char buf[256] = "";
  char numbuf[21];
  append_string(buf, label, sizeof(buf));
  if (hex)
    itoa_hex(v, numbuf);
  else
    itoa(v, numbuf);
  append_string(buf, numbuf, sizeof(buf));
  append_string(buf, "\n", sizeof(buf));
  message_send_message(buf);
}

void vbe_init(fb_surface_t *target) {
  uint32_t bytes_per_pixel = (handoff_kerncopy.vbe_bpp + 7) / 8;

  message_send_message(" vbe:");
  vbe_log_u32(" framebuffer at: ", handoff_kerncopy.vbe_framebuffer, true);
  vbe_log_u32(" pitch=", handoff_kerncopy.vbe_pitch, false);
  vbe_log_u32(" width=", handoff_kerncopy.vbe_width, false);
  vbe_log_u32(" height=", handoff_kerncopy.vbe_height, false);
  vbe_log_u32(" bpp=", handoff_kerncopy.vbe_bpp, false);
  message_send_message("\n");

  uint32_t physical_base = handoff_kerncopy.vbe_framebuffer & ~0xFFFu;
  uint32_t page_offset = handoff_kerncopy.vbe_framebuffer & 0xFFFu;
  uint32_t framebuffer_size =
      handoff_kerncopy.vbe_pitch * handoff_kerncopy.vbe_height;

  // some safety... 安全 or smth idk safoihasofihofas
  if (handoff_kerncopy.vbe_width == 0 || handoff_kerncopy.vbe_height == 0 ||
      (handoff_kerncopy.vbe_bpp != 15 && handoff_kerncopy.vbe_bpp != 16 &&
       handoff_kerncopy.vbe_bpp != 24 && handoff_kerncopy.vbe_bpp != 32) ||
      handoff_kerncopy.vbe_pitch <
          handoff_kerncopy.vbe_width * bytes_per_pixel ||
      framebuffer_size == 0 || framebuffer_size > 0x04000000u ||
      handoff_kerncopy.vbe_framebuffer > 0xFFFFFFFFu - framebuffer_size ||
      handoff_kerncopy.vbe_red_mask > 8 ||
      handoff_kerncopy.vbe_green_mask > 8 ||
      handoff_kerncopy.vbe_blue_mask > 8 ||
      handoff_kerncopy.vbe_red_position > 31 ||
      handoff_kerncopy.vbe_green_position > 31 ||
      handoff_kerncopy.vbe_blue_position > 31) {
    message_send_message(" vbe: invalid mode info, not initializing\n");
    return;
  }

  // optimisation - make it compile faster
  fmt.rpos = handoff_kerncopy.vbe_red_position;
  fmt.gpos = handoff_kerncopy.vbe_green_position;
  fmt.bpos = handoff_kerncopy.vbe_blue_position;
  fmt.rlose = 8 - handoff_kerncopy.vbe_red_mask;
  fmt.glose = 8 - handoff_kerncopy.vbe_green_mask;
  fmt.blose = 8 - handoff_kerncopy.vbe_blue_mask;
  fmt.bpp_bytes = (uint8_t)bytes_per_pixel;
  fmt.pitch = handoff_kerncopy.vbe_pitch;
  fmt.width = handoff_kerncopy.vbe_width;
  fmt.height = handoff_kerncopy.vbe_height;
  fmt.size = framebuffer_size;

  uint32_t map_len = (framebuffer_size + page_offset + 0xFFFu) & ~0xFFFu;

  // so basically we ignore if MTRR cant work cos like issok either way
  bool_t wc_ok = (mtrr_init() == 0);
  if (!wc_ok)
    message_send_message(" vbe: could not init mtrr\n");

  if (wc_ok) {
    uint32_t p = physical_base;
    uint32_t remaining = map_len;

    while (remaining != 0) {
      uint32_t size = VMM_PAGE_SIZE;
      while (size <= (remaining >> 1) && (p & ((size << 1) - 1)) == 0)
        size <<= 1;

      if (mtrr_set_region(p, size, MTRR_TYPE_WC) != 0) {
        message_send_message(" vbe: could not set region to WC\n");
        wc_ok = false;
        break;
      }
      p += size;
      remaining -= size;
    }
  }

  uint32_t map_flags = wc_ok ? VMM_PAGE_PRESENT_RW
                             : (VMM_PAGE_PRESENT_RW | VMM_PAGE_CACHE_DISABLE);

  uint32_t offset = 0;

  if ((physical_base & (VBE_4MB - 1)) == 0) {
    while (map_len - offset >= VBE_4MB) {
      vmm_map_phys_4mb(vmm_current_pagemap(), physical_base + offset,
                       VBE_VIRT_BASE + offset, map_flags);
      offset += VBE_4MB;
    }
  }

  while (offset < map_len) {
    vmm_map_phys_to_virt_flags(vmm_current_pagemap(), physical_base + offset,
                               VBE_VIRT_BASE + offset, map_flags);
    offset += VMM_PAGE_SIZE;
  }

  if (wc_ok)
    mtrr_enable(true);
  else
    message_send_message(" vbe: failed to configure MTRR for framebuffer\n");

  vbe_framebuffer = (volatile uint8_t *)(VBE_VIRT_BASE + page_offset);

  vbe_2buf = kmalloc(framebuffer_size);
  if (vbe_2buf == 0) {
    message_send_message(" vbe: failed to allocate 2nd framebuffer\n");
    vbe_2buf = (uint8_t *)vbe_framebuffer;
    vbe_direct = true; // draw straight to the card; flush becomes a no-op
  } else {
    memset(vbe_2buf, 0, framebuffer_size);
    vbe_direct = false;
  }

  dirty_reset();

  vbe_log_u32(" vbe: DBuf @ ", (uint32_t)vbe_2buf, true);

  target->height = fmt.height;
  target->width = fmt.width;
  target->write_pixel = vbe_put_pixel;
  target->scroll_up = vbe_scroll_up;
  target->clear = vbe_clear;
  target->blit = vbe_blit_rgba;
  target->fill_rect = vbe_fill_rect;
  target->update_fb = vbe_update_fb;

  message_send_message(" vbe: initf\n");
}
