#ifndef DISK_H
#define DISK_H
#include "../kernel_types.h"

#define DISK_MAX 26
#define DISK_ERR_HW 0xFF
#define DISK_ERR_PERM 0xFE
#define DISK_ERR_RANGE 0xFD

typedef struct {
  uint8_t attr;
  uint8_t type;
  uint32_t lba_start;
  uint32_t size; // in sectors
} partition_metadata_t;

typedef struct disk_info {
  char pretty_name[16];
  bool_t present;
  uint32_t resv;
  uint8_t (*write)(struct disk_info *self, uint32_t lba, uint32_t n, void *from);
  uint8_t (*read)(struct disk_info *self, uint32_t lba, uint32_t n, void *to);
  bool_t is_mbr;
  partition_metadata_t partition_metadata[4];
  uint32_t size; // in 512-byte sectors
} disk_info_t;

typedef struct partition_info {
  char pretty_name[16];
  bool_t present;
  uint32_t resv;
  disk_info_t *parent;
  uint8_t part_n;
  uint8_t (*write)(struct partition_info *self, const char *filename, void *from);
  uint8_t (*read)(struct partition_info *self, const char *filename, void *to);
} partition_info_t;

typedef struct {
  bool_t ide;
} disk_featuremask_t;
void disk_init(void);
uint8_t disk_get_by_prettyname(const char *pretty_name);
disk_info_t *disk_get_disk(uint8_t n);
#endif
