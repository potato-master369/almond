// kernel main.c
// --------------------------
#include "../boot/handoff_struct.h"
#include "helpers/string.h"
#include "idt/idt.h"
#include "messaging/messaging.h"
#include "mmu/e820.h"
#include "mmu/kmalloc.h"
#include "mmu/pmm.h"
#include "mmu/vmm.h"
#include "framebuffer/fb.h"
#include "framebuffer/bsman.h"
#include "panic.h"
#include "cmdline.h"
#include "disk/disk.h"
#include "pci/pci.h"

extern fb_surface_t fb0;
extern const uint32_t bootsplash_img[];

core_featuremask_t core_featuremask = {
  .bsman = false
};
almond_handoff_t handoff_kerncopy;
void kmain(almond_handoff_t *f) {
  if (f->magic[0] != 'a' || f->magic[1] != 'l' || f->magic[2] != 'm' ||
      f->magic[3] != 'o' || f->magic[4] != 'n' || f->magic[5] != 'd') {
    message_send_message(" kmain: invalid magic\n");
    panic_printa(0xD001, "INVALID_MAGIC");
  }
  handoff_kerncopy = *f;
  message_init();
  idt_init();
  message_send_message(" kmain: cmdline: ");
  message_send_message(handoff_kerncopy.cmdline);
  message_send_message("\n");
  cmdline_parse_cmdline(handoff_kerncopy.cmdline);
  char buf[64];
  itoa((int)(handoff_kerncopy.mmap_ptr), buf);
  message_send_message(" kmain: mmap pointer at ");
  message_send_message(buf);
  message_send_message("\n");
  message_send_message(buf);
  pmm_process_e820(handoff_kerncopy.mmap_ptr, handoff_kerncopy.mmap_count);
  vmm_init();
  kmalloc_init();
  fb_init();
  for (uint32_t x = 0; x < fb0.width; ++x) {
    for (uint32_t y = 0; y < fb0.height; ++y) {
      fb0.write_pixel(x, y, 0x4a4a9900, &fb0);
    }
  }
  for (int y = 0; y < 480; ++y) {
    for (int x = 0; x < 640; ++x) {
      fb0.write_pixel((fb0.width / 2 - 320) + x, (fb0.height / 2 - 240) + y, bootsplash_img[y * 640 + x], &fb0);
    }
  }
  for (uint32_t y = fb0.height - 16; y < fb0.height; ++y) {
    for (uint32_t x = 0; x < fb0.width; ++x) {
      fb0.write_pixel(x, y, 0xffffffff, &fb0);
    }
  }
  fb0.update_fb(&fb0);
  if (!core_featuremask.bsman)
    bsman_init();
  message_send_message(" kmain: quack!\n");
  pci_init();
  disk_init();
  // test
  disk_info_t *a = disk_get_disk(disk_get_by_prettyname("ide0"));
  message_send_message("asfpksafhkasfhj\n");
  message_send_message(a->pretty_name);
  uint8_t testbuf[512];
  message_send_message("asfpksafhkasfhj\n");
  uint8_t res = a->read(a, 0, 1, testbuf);
  if (res == 0) {
	  message_send_message(" test complete\n");
	  if (testbuf[510] == 0x55 && testbuf[511] == 0xAA) {
            message_send_message(" MBR checksum PASS!\n");
	  }
  }
  else if (res == DISK_ERR_HW) {
	  message_send_message(" test fail: HW\n"); 
  } else {
          message_send_message("OTHER");
  }
  for (;;)
    ;
}

