// disk.c
// ---------------------
// Disk subsystem for Almond
#include "../kernel_types.h"
#include "disk.h"
#include "../messaging/messaging.h"
#include "../helpers/string.h"

// drivers
#include "drv/ide.h"

// these should be set in kmain
disk_featuremask_t disk_drivermasks = {
  .ide = false
};

disk_info_t disk_db[DISK_MAX]; // fxck unix
partition_info_t part_db[26]; // A:/, B:/ etc

void disk_init(void) {
  message_send_message(" disk: init\n");
  if (!disk_drivermasks.ide) {
    ide_init(disk_db);
  }
  message_send_message(" disk: init finish\n");
}

uint8_t disk_get_by_prettyname(const char *pretty_name) {
  for (int i = 0; i < DISK_MAX; ++i) {
    if(strncmp(disk_db[i].pretty_name, pretty_name, sizeof(disk_db[i].pretty_name)) == 0) {
      return i;
    }
  }
  return 255;
}

disk_info_t *disk_get_disk(uint8_t n) {
  return &(disk_db[n]);
}
