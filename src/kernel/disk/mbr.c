// mbr.c
// ---------------------------------------
// Partition Table handler
//
// Although GPT is largely prefered for modern
// partitioning, we try to be as simple as possible
// and just do it using MBR.
//
// Additionally, GPT would mean using heap which,
// as of now, can't really be trusted.
#include "disk.h"
#include "../kernel_types.h"
#include "../messaging/messaging.h"

// constants
extern disk_info_t disk_db[DISK_MAX]; // fxck unix

// takes the disk to init as a ptr
void mbr_init_disk(disk_info_t *a) {
  uint8_t scratch[512];
  uint8_t s = a->read(a, 0, 1, scratch); // read the first sector
  if (s == DISK_ERR_HW || s == DISK_ERR_PERM || s == DISK_ERR_RANGE)
	  return; // disk errored out, scratch contains garbage!
  if (scratch[0x1fe] == 0x55 && scratch[0x1ff] == 0xAA) {
    message_send_message(" mbr: found!\n");
    a->is_mbr = true;
  } else {
    a->is_mbr = false;
    return;
  }
  // MBR partition entries are always 16 bytes so its easy for us to
  // do this!
  for (int i = 0; i < 4; ++i) {
    a->partition_metadata[i].attr = scratch[(0x1BE + (i * 16))]; // partition i
    a->partition_metadata[i].type = scratch[(0x1BE + (i * 16)) + 0x04];
    a->partition_metadata[i].lba_start = 
        (uint32_t)scratch[(0x1BE + (i * 16)) + 0x08] |
        ((uint32_t)scratch[(0x1BE + (i * 16)) + 0x09] << 8) |
        ((uint32_t)scratch[(0x1BE + (i * 16)) + 0x0A] << 16) |
        ((uint32_t)scratch[(0x1BE + (i * 16)) + 0x0B] << 24);
        
    a->partition_metadata[i].size = 
        (uint32_t)scratch[(0x1BE + (i * 16)) + 0x0C] |
        ((uint32_t)scratch[(0x1BE + (i * 16)) + 0x0D] << 8) |
        ((uint32_t)scratch[(0x1BE + (i * 16)) + 0x0E] << 16) |
        ((uint32_t)scratch[(0x1BE + (i * 16)) + 0x0F] << 24);
  }
}
