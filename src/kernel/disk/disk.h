#ifndef DISK_H
#define DISK_H
#include "../kernel_types.h"

#define DISK_MAX 26
#define DISK_ERR_HW 0xFF;
#define DISK_ERR_PERM 0xFE;
#define DISK_ERR_RANGE 0xFD;

typedef struct disk_info {
  char pretty_name[16];
  bool_t present;
  uint32_t resv;
  uint8_t (*write)(struct disk_info *self, uint32_t lba, uint32_t n, void *to);
  uint8_t (*read)(struct disk_info *self, uint32_t lba, uint32_t n, void *from);
  bool_t is_mbr;
  uint32_t part_starts[4];
  uint32_t part_ends[4];
  uint32_t size; // in 512-byte sectors
} disk_info_t;

typedef struct partition_info {
  char pretty_name[16];
  bool_t present;
  uint32_t resv;
  disk_info_t *parent;
  uint8_t part_n;
  uint8_t (*write)(struct partition_info *self, const char *filename, void *to);
  uint8_t (*read)(struct partition_info *self, const char *filename, void *from);
} partition_info_t;

typedef struct {
  bool_t ide;
} disk_featuremask_t;
void disk_init(void);
#endif
