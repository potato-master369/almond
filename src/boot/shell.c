// shell.c
// --------------------
// MIT License; Copyright (C) 2026-
// ABLR shell
#include "bl_vga.h"
#include "drv/disk.h"
#include "drv/keyboard.h"
#include "handoff_struct.h"
#include "string.h"

int s_cur_pos = 0;
char cmdbuffer[256];
almond_handoff_t f;

// hacky shit to get access to disk_sel and part_sel
extern int disk_sel;
extern int part_sel;
extern disk_t disk_info[DISK_MAX];
// hacky shit for our mmap stuff
extern uint16_t mmap_count;
extern void *mmap_ptr; // should still be valid after hh jump

void run_command(void) {
  if (bl_strncmp(cmdbuffer, "help", 256) == 0) {
    bl_vga_write(" ABLR: Help\n\n reboot: Reboot the computer\n help: Display "
                 "this message\n lsdsk: list disks\n lspart: list partitions\n "
                 "sel disk N: select disk N\n sel part N: select partition N\n"
                 " ucat PATH: display file contents\n set cmdline CMDLINE: set "
                 "kernel command line\n loadkern PATH: load kernel from PATH\n "
                 "boot: boot the loaded kernel\n",
                 0x07);
    return;
  } else if (bl_strncmp(cmdbuffer, "reboot", 256) == 0) {
    bl_vga_write(" ABLR: Goodbye!\n", 0x07);
    keyboard_reboot();
  } else if (bl_strncmp(cmdbuffer, "lsdsk", 256) == 0) {
    shell_list_disk();
  } else if (bl_strncmp(cmdbuffer, "lspart", 256) == 0) {
    shell_list_part();
  } else if (bl_strncmp(cmdbuffer, "sel disk ", 9) == 0) {
    // parse the number following "sel disk "
    const char *p = cmdbuffer + 9;

    if (*p < '0' || *p > '9') {
      bl_vga_write(" ABLR: usage: sel disk N\n", 0x07);
      return;
    }

    uint32_t n = 0;
    while (*p >= '0' && *p <= '9') {
      n = (n * 10) + (*p - '0');
      p++;

      if (n > 255) {
        bl_vga_write(" ABLR: disk number out of range (0-255)\n", 0x07);
        return;
      }
    }

    disk_sel_disk((uint8_t)n);
    return;
  } else if (bl_strncmp(cmdbuffer, "sel part ", 9) == 0) {
    // parse the number following "sel disk "
    const char *p = cmdbuffer + 9;

    if (*p < '0' || *p > '9') {
      bl_vga_write(" ABLR: usage: sel part N\n", 0x07);
      return;
    }

    uint32_t n = 0;
    while (*p >= '0' && *p <= '9') {
      n = (n * 10) + (*p - '0');
      p++;

      if (n > 255) {
        bl_vga_write(" ABLR: part number out of range (0-255)\n", 0x07);
        return;
      }
    }

    disk_sel_part((uint8_t)n);
    return;
  } else if (bl_strncmp(cmdbuffer, "ucat ", 5) == 0) {
    const char *p = cmdbuffer + 5;
    shell_ucat(p);
  } else if (bl_strncmp(cmdbuffer, "set cmdline ", 12) == 0) {
    const char *p = cmdbuffer + 12;
    bl_strcpy(f.cmdline, p);
  } else if (bl_strncmp(cmdbuffer, "loadkern ", 9) == 0) {
    const char *p = cmdbuffer + 9;
    disk_load_kern(disk_sel, part_sel, p);
  } else if (bl_strncmp(cmdbuffer, "boot", 4) == 0) {
    f.magic[0] = 'a';
    f.magic[1] = 'l';
    f.magic[2] = 'm';
    f.magic[3] = 'o';
    f.magic[4] = 'n';
    f.magic[5] = 'd';
    f.mmap_count = mmap_count;
    f.mmap_ptr = mmap_ptr;
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

  } else {
    bl_vga_write(" ABLR: Unknown command\n", 0x07);
    return;
  }
}
void awaitbuf(void) {
  for (;;) {
    unsigned char k = keyboard_read_scancode();
    keyboard_mod_state(k);
    switch (k) {
    case 0x1C:
      cmdbuffer[s_cur_pos] = '\0';
      bl_vga_write("\n", 0x07);
      s_cur_pos = 0;
      run_command();
      return;
    case 0x0E:
      if (s_cur_pos > 0) {
        --s_cur_pos;
        bl_vga_backspace(0x07);
      }
      break;
    default:
      if (keyboard_get_ascii(k) != 0) {
        cmdbuffer[s_cur_pos] = keyboard_get_ascii(k);
        bl_vga_write_1(keyboard_get_ascii(k), 0x07);
        ++s_cur_pos;
      }
      break;
    }
  }
}

void setkeymap(void) {
  bl_vga_write(" ABLR: Select keymap\n\n\t\t0: US Layout   \t\t\t\t\t  \t\t1: "
               "JIS Layout\n\t\t2: ISO (international English)\n\n ABLR:",
               0x07);
  unsigned char k = keyboard_read_scancode();
  while (k != 0x0B && k != 0x02 && k != 0x03) {
    k = keyboard_read_scancode();
  }
  switch (k) {
  case 0x0B:
    keyboard_set_keymap(0);
    break;
  case 0x02:
    keyboard_set_keymap(1);
    break;
  case 0x03:
    keyboard_set_keymap(2);
    break;
  }
  bl_vga_write("\n", 0x07);
}
