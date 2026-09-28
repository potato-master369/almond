#ifndef DISK_H
#define DISK_H
#include "../bl_types.h"
// struct for a disk
typedef struct {
  uint16_t drv_id;
  uint8_t (*read_sec)(uint32_t sector_offset, void *buf, uint8_t drv_id);
  uint8_t (*write_sec)(uint32_t sector_offset, void *buf, uint8_t drv_id);
  char pretty_name[8];
  bool_t is_mbr;
  uint8_t mbr[4][16];
  uint8_t (*part_read[4])(uint8_t disk_n, uint8_t part_n, const char *path, void *buf, uint32_t read_size);
  uint8_t (*part_write[4])(uint8_t disk_n, uint8_t part_n, const char *path, void *buf, uint32_t read_size);
  int32_t (*part_get_filesize[4])(uint8_t disk_n, uint8_t part_n, const char *path);
  bool_t is_almond;
} disk_t;

#define DRV_ID_NULL 0
#define DISK_MAX 8

void disk_init(void);
void disk_discover(void);
void shell_list_disk(void);
void shell_list_part(void);
void shell_ucat(const char *path);
void disk_sel_disk(uint8_t disk);
void disk_sel_part(uint8_t part);
uint8_t disk_self_discover(uint8_t disk_n);
uint8_t disk_autoload_kern(uint8_t disk_n);
// the following functions return 0x10 if invalid partition and 0x11 if invalid offset
uint8_t disk_read_w_part(uint8_t disk_n, uint8_t part_n, uint32_t offset, void *buf);
uint8_t disk_write_w_part(uint8_t disk_n, uint8_t part_n, uint32_t offset, void *buf);
// load kernel
void disk_load_kern(uint8_t disk_n, uint8_t part_n, const char *path);
#endif
