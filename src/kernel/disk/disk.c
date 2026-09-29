// disk.c
// ---------------------
// Disk subsystem for Almond
#include "../kernel_types.h"
#include "disk.h"
#include "../messaging/messaging.h"

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
}
