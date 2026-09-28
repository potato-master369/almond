// stage2.c
// -----------------------------
// Stage 2 bootloader. This has to fit
// in 31.5KiB, so it's much of a less
// painful fit than the BS.
#include "bl_types.h"
#define BL_PORTS_IMPL
#include "bl_ports.h"
#include "bl_vga.h"
#include "drv/disk.h"
#include "drv/keyboard.h"
#include "handoff_struct.h"
#include "shell.h"
#include "string.h"

extern disk_t disk_info[DISK_MAX];

// make GCC shut the fuck up
void *memset(void *bufptr, int value, int32_t size) {
  unsigned char *buf = (unsigned char *)bufptr;
  for (int32_t i = 0; i < size; i++) {
    buf[i] = (unsigned char)value;
  }
  return bufptr;
}

extern uint8_t mmap_buf[1536];
extern uint16_t mmap_count;
void *mmap_ptr = mmap_buf;

int16_t bl_main(void) {
  // save memory map pointers
  bl_vga_init();
  bl_vga_set_x(0);
  bl_vga_write(" ABL: quack! a keyboard available through P/S2 is required to "
               "continue.\n",
               0x07);
mainmenu:
  bl_vga_write("\n\t\t0: Boot Almond\t\t\t\t\t  \t\t1: Diagnostics\n\n ABL: ",
               0x07);
  unsigned char k;
  k = keyboard_read_scancode();
  while (k != 0x0B && k != 0x02) {
    k = keyboard_read_scancode();
  }
  switch (k) {
  case 0x0B:
    // boot almond
    bl_vga_write("\n ABL: quack!\n", 0x07);
    disk_init();
    disk_discover();

    // self discover
    for (uint8_t disk_n = 0; disk_n < DISK_MAX; ++disk_n) {
      if (disk_self_discover(disk_n) == 0) {
        // this is our bootloader
        uint8_t part_n = disk_autoload_kern(disk_n);
        if (part_n == 255) {
          bl_vga_write(" ABL: kernel autoload failed.\n", 0x04);
          goto mainmenu;
        }
        almond_handoff_t f;
        char buf[512] = "root=";
        append_string(buf, disk_info[disk_n].pretty_name, sizeof(buf));
        append_string(buf, ",", sizeof(buf));
        append_uint(buf, (uint32_t)part_n, sizeof(buf));
        bl_strcpy(f.cmdline, buf);
        f.magic[0] = 'a';
        f.magic[1] = 'l';
        f.magic[2] = 'm';
        f.magic[3] = 'o';
        f.magic[4] = 'n';
        f.magic[5] = 'd';
        f.mmap_count = mmap_count;
        f.mmap_ptr = mmap_ptr;
        f.vbe_width = bl_vbe_width;
        f.vbe_height = bl_vbe_height;
        f.vbe_pitch = bl_vbe_pitch;
        f.vbe_framebuffer = bl_vbe_framebuffer;
        f.vbe_bpp = bl_vbe_bpp;
        f.vbe_red_mask = bl_vbe_red_mask;
        f.vbe_red_position = bl_vbe_red_position;
        f.vbe_blue_mask = bl_vbe_blue_mask;
        f.vbe_blue_position = bl_vbe_blue_position;
        f.vbe_green_mask = bl_vbe_green_mask;
        f.vbe_green_position = bl_vbe_green_position;
        bl_vga_write(" main: jumping to kernel (1MiB)...\n", 0x07);
        void (*entry)(almond_handoff_t *) =
            (void (*)(almond_handoff_t *))(void *)0x100000;
        entry(&f);
      }
      break;
    }
    break;
  case 0x02:
    // diagnostics
    bl_vga_write(
        "\n\n\t\t0: ABLR Shell \t\t\t\t\t  \t\t1: previous menu\n\n ABL: ",
        0x07);
    goto diagnostics;
  default:
    bl_vga_write("\n ABL: Invalid input. Returning to previous menu.\n", 0x07);
    goto mainmenu;
  }
  for (;;)
    __asm__ __volatile__("hlt");
diagnostics:
  // diagnostics
  k = keyboard_read_scancode();
  while (k != 0x0B && k != 0x02) {
    k = keyboard_read_scancode();
  }
  switch (k) {
  case 0x0B:
    // shell
    bl_vga_write(" ABLR: Starting shell\n", 0x07);
    disk_init();
    disk_discover();
    bl_vga_write(" ABLR: The ABLR shell is used to do recovery tasks like "
                 "manually editing text files or issuing hardware commands.\n",
                 0x02);
    // shell loop
    setkeymap();
    bl_vga_write(" ABLR Shell\n", 0x02);
    for (;;) {
      bl_vga_write("> ", 0x07);
      awaitbuf();
    }
    break;
  case 0x02:
    goto mainmenu;
  }
  return 0;
}
