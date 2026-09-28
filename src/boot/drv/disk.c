#include "disk.h"
// drivers
#include "../bl_vga.h"
#include "../string.h"
#include "ahci.h"
#include "fdd.h"
#include "ide.h"
// filesystems
#include "ext2.h"
#include "fat32.h"

// macro for null
#define NULL (void *)0

// store all our disk information
disk_t disk_info[DISK_MAX]; // by default 8
disk_t *disk_info_p = disk_info;

int disk_sel = 0;
int part_sel = 0;
// discover and populate disk info
void disk_discover(void) {
  bl_vga_write(" disk: popuating disk_info\n", 0x07);
  // example: driver_discover(&disk_info)
  ide_discover(&disk_info_p);
  bl_vga_write(" disk: reading MBRs\n", 0x07);
  uint8_t buf[512];
  for (int i = 0; i < DISK_MAX; ++i) {
    if (disk_info[i].drv_id != DRV_ID_NULL) {
      // read the sector
      disk_info[i].read_sec(0, buf, disk_info[i].drv_id);
      if (buf[510] == 0x55 && buf[511] == 0xAA) {
        // valid MBR
        disk_info[i].is_mbr = true;
        bl_vga_write(" disk: read MBR\n", 0x07);
        for (int j = 0; j < 4; ++j) {
          for (int k = 0; k < 16; ++k) {
            disk_info[i].mbr[j][k] = buf[0x1BE + (j * 16) + k];
          }
	  ext2_verify(i, j);
        }
      }
    }
  }
}

void disk_init(void) {
  bl_vga_write(" disk: clear disk_info\n", 0x07);
  for (int i = 0; i < DISK_MAX; ++i) {
    disk_info[i].drv_id = DRV_ID_NULL;
    disk_info[i].read_sec = NULL;
    disk_info[i].write_sec = NULL;
    disk_info[i].pretty_name[0] = 'N';
    disk_info[i].pretty_name[1] = 'U';
    disk_info[i].pretty_name[2] = 'L';
    disk_info[i].pretty_name[3] = 'L';
    disk_info[i].pretty_name[4] = '\0';
    disk_info[i].is_mbr = false;
  }
}

void shell_list_disk(void) {
  bl_vga_write("\n disk list\n", 0x07);
  char buf[256];
  for (int i = 0; i < DISK_MAX; ++i) {
    itoa(i, buf);
    bl_vga_write(buf, 0x07);
    bl_vga_write(" id: ", 0x07);
    itoa(disk_info[i].drv_id, buf);
    bl_vga_write(buf, 0x0F);
    bl_vga_write(" pretty name: ", 0x07);
    bl_vga_write(disk_info[i].pretty_name, 0x0F);
    bl_vga_write("\n", 0x07);
  }
}

uint8_t disk_self_discover(uint8_t disk_n) {
  char buf[512];
  disk_info[disk_n].read_sec(63, (void*)buf, disk_info[disk_n].drv_id);
  if (buf[506] == 'a' && buf[507] == 'l' && buf[508] == 'm' && buf[509] == 'o' && buf[510] == 'n' && buf[511] == 'd') {
    bl_vga_write(" disk: self discovered\n", 0x07);
    return 0;
  }
  return 1;
}

uint8_t disk_autoload_kern(uint8_t disk_n) {
  for (uint8_t i = 0; i < 4;++i) {
    if (disk_info[disk_n].part_get_filesize[i] != NULL && disk_info[disk_n].part_read[i] != NULL) {
      int32_t filesize = disk_info[disk_n].part_get_filesize[i](
          disk_n, i, "/boot/almond.bin");
      if (filesize > 0) {
        if (disk_info[disk_n].part_read[i](disk_n, i, "/boot/almond.bin",
                                           (void *)0x100000,
                                           (uint32_t)filesize) != 0x10) {
          bl_vga_write(" disk: kernel autoload success\n", 0x07);
	  return i;
	} else {
          bl_vga_write(" disk: read failure\n", 0x04);
	}
      } else if (filesize == 0) {
        bl_vga_write(" disk: kernel file is empty\n", 0x04);
      }
    }
  }
  return 255;
}
// test functions
#ifndef ALM_NO_TEST

#endif
void shell_list_part(void) {
  if (disk_info[disk_sel].drv_id == DRV_ID_NULL) {
    bl_vga_write(" disk: disk not present\n", 0x07);
    return;
  } else if (!disk_info[disk_sel].is_mbr) {
    bl_vga_write(" disk: disk not MBR\n", 0x07);
    return;
  } else {
    // lspart
    char buf[256];
    for (int i = 0; i < 4; ++i) {
      if (disk_info[disk_sel].mbr[i][4] != 0) {
        itoa(i, buf);
        bl_vga_write(buf, 0x07);
        bl_vga_write(" type: ", 0x07);
        itoa(disk_info[disk_sel].mbr[i][4], buf);
        bl_vga_write(buf, 0x0B);
        bl_vga_write(" starts at: ", 0x07);
        uint8_t *entry = disk_info[disk_sel].mbr[i];
        uint32_t start_lba =
            ((uint32_t)entry[0x0B] << 24) | ((uint32_t)entry[0x0A] << 16) |
            ((uint32_t)entry[0x09] << 8) | (uint32_t)entry[0x08];
        itoa(start_lba, buf);
        bl_vga_write(buf, 0x0F);
        bl_vga_write(" size: ", 0x07);
        uint32_t total_sectors =
            ((uint32_t)entry[0x0F] << 24) | ((uint32_t)entry[0x0E] << 16) |
            ((uint32_t)entry[0x0D] << 8) | (uint32_t)entry[0x0C];
        itoa(total_sectors, buf);
        bl_vga_write(buf, 0x0F);
        bl_vga_write("\n", 0x07);
      } else {
        itoa(i, buf);
        bl_vga_write(buf, 0x07);
        bl_vga_write(" NULL\n", 0x0F);
      }
    }
  }
}

void shell_ucat(const char *path) {
  if (disk_info[disk_sel].drv_id == DRV_ID_NULL) {
    bl_vga_write(" disk: disk not present\n", 0x07);
    return;
  } else if (!disk_info[disk_sel].is_mbr) {
    bl_vga_write(" disk: disk not MBR\n", 0x07);
    return;
  } else if (disk_info[disk_sel].part_read[part_sel] == NULL || disk_info[disk_sel].part_get_filesize[part_sel] == NULL) {
    bl_vga_write(" disk: partition not assigned driver\n", 0x07);
  }else {
	  int32_t filesize = disk_info[disk_sel].part_get_filesize[part_sel](disk_sel, part_sel, path);
	  if (filesize == -1) {
            bl_vga_write(" disk: no such file\n", 0x07);
	  } else if (filesize == 0) {
            bl_vga_write(" disk: file is empty\n", 0x07);
	  }
	  char buf[filesize + 1];
	  if (disk_info[disk_sel].part_read[part_sel](disk_sel, part_sel, path, (void*)buf, filesize) == 0x10) {
            bl_vga_write(" disk: read error\n", 0x04);
	  };
	  buf[filesize] = '\0';
	  bl_vga_write(buf, 0x07);
  }
}

void disk_sel_disk(uint8_t disk) {
  if (disk > DISK_MAX) {
    bl_vga_write(" disk: out of range\n", 0x04);
    return;
  }
  disk_sel = disk;
}

void disk_sel_part(uint8_t part) {
  if (part > 4) {
    bl_vga_write(" disk: out of range\n", 0x04);
    return;
  }
  part_sel = part;
}

void disk_load_kern(uint8_t disk_n, uint8_t part_n, const char *path) {
  if (disk_n >= DISK_MAX || part_n >= 4) {
    bl_vga_write(" disk: selection out of range\n", 0x04);
    return;
  } else if (disk_info[disk_n].drv_id == DRV_ID_NULL) {
    bl_vga_write(" disk: disk not present\n", 0x07);
    return;
  } else if (!disk_info[disk_n].is_mbr) {
    bl_vga_write(" disk: disk not MBR\n", 0x07);
    return;
  } else if (disk_info[disk_n].part_read[part_n] == NULL ||
             disk_info[disk_n].part_get_filesize[part_n] == NULL) {
    bl_vga_write(" disk: partition not assigned driver\n", 0x07);
    return;
  } else {
    int32_t filesize = disk_info[disk_n].part_get_filesize[part_n](disk_n, part_n, path);
    char buf[64];
    itoa(filesize, buf);
    bl_vga_write("filesize: ", 0x07);
    bl_vga_write(buf, 0x0F);
    bl_vga_write("\n", 0x07);
    if (filesize == -1) {
      bl_vga_write(" disk: no such file\n", 0x07);
      return;
    } else if (filesize == 0) {
      bl_vga_write(" disk: file is empty\n", 0x07);
      return;
    }
    // load to 1MiB mark
    if (disk_info[disk_n].part_read[part_n](disk_n, part_n, path, (void*)0x00100000, filesize) == 0x10) {
      bl_vga_write(" disk: read error\n", 0x04);
    }
  }
}

uint8_t disk_read_w_part(uint8_t disk_n, uint8_t part_n, uint32_t offset,
                         void *buf) {
  uint8_t *entry = (disk_n >= DISK_MAX || part_n >= 4) ? disk_info[0].mbr[0] : disk_info[disk_n].mbr[part_n];
  if (entry[4] == 0 || disk_n >= DISK_MAX || part_n >= 4) {
    bl_vga_write("W7010: Invalid partition\n", 0x04);
    return 0x10;
  }

  uint32_t start_lba = ((uint32_t)entry[0x0B] << 24) |
                       ((uint32_t)entry[0x0A] << 16) |
                       ((uint32_t)entry[0x09] << 8) | (uint32_t)entry[0x08];
  uint32_t total_sectors = ((uint32_t)entry[0x0F] << 24) |
                           ((uint32_t)entry[0x0E] << 16) |
                           ((uint32_t)entry[0x0D] << 8) | (uint32_t)entry[0x0C];
  if (offset >= total_sectors) {
    bl_vga_write("W7011: Invalid offset\n", 0x04);
    return 0x11;
  }
  disk_info[disk_n].read_sec(start_lba + offset, buf, disk_info[disk_n].drv_id);
  return 0;
}
uint8_t disk_write_w_part(uint8_t disk_n, uint8_t part_n, uint32_t offset,
                          void *buf) {
  uint8_t *entry = (disk_n >= DISK_MAX || part_n >= 4) ? disk_info[0].mbr[0] : disk_info[disk_n].mbr[part_n];
  if (entry[4] == 0 || disk_n >= DISK_MAX || part_n >= 4) {
    bl_vga_write("W7010: Invalid partition\n", 0x04);
    return 0x10;
  }

  uint32_t start_lba = ((uint32_t)entry[0x0B] << 24) |
                       ((uint32_t)entry[0x0A] << 16) |
                       ((uint32_t)entry[0x09] << 8) | (uint32_t)entry[0x08];
  uint32_t total_sectors = ((uint32_t)entry[0x0F] << 24) |
                           ((uint32_t)entry[0x0E] << 16) |
                           ((uint32_t)entry[0x0D] << 8) | (uint32_t)entry[0x0C];
  if (offset >= total_sectors) {
    bl_vga_write("W7011: Invalid offset\n", 0x04);
    return 0x11;
  }
  disk_info[disk_n].write_sec(start_lba + offset, buf, disk_info[disk_n].drv_id);
  return 0;
}
