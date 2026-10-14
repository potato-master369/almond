// ext2.c
// ---------------------------
// Kernel glue code for ext2
//
// Basically i decide what to steal later
// and then put that ONTO this mechanism
#include "ext2.h"
#include "kernel_types.h"
#include "messaging/messaging.h"
#include "mmu/kmalloc.h"
#include "disk/disk.h"
#include "disk/part/part.h"
#include "include/def.h"

int ext2_write(almond_filehandle_t *fh, void *from, int n) { return 0; }
int ext2_read(almond_filehandle_t *fh, void *to, int n) { return 0; }
int ext2_openstream(almond_filehandle_t *fh, const char *filename) { return 0; }
int ext2_closestream(almond_filehandle_t *fh) { return 0; }
int ext2_stat(almond_stat_t *out, const char *path) { return 0; }
int ext2_dir_index(almond_filehandle_t *dir, uint32_t index,
                   almond_dirent_t *out) {
  return 0;
}
int ext2_dir_count(almond_filehandle_t *dir, uint32_t *out) { return 0; }

int ext2_mount_part(partition_info_t *target, disk_info_t *parent,
                    uint8_t part_n) {
  ext2_superblock_full_t *sb = kmalloc(sizeof(ext2_superblock_full_t));
  if (!sb) {
    message_send_message(" ext2: mount kmalloc fail!\n");
    return false;
  }
  parent->read(parent, parent->partition_metadata[part_n].lba_start + 2, 2, sb);
  // check the signature of EXT2
  if (sb->main.sig == 0xEF53) {
    message_send_message(" ext2: found!\n");
    target->parent = parent;
    target->read = ext2_read;
    target->write = ext2_write;
    target->resv = (uint32_t)kmalloc(sizeof(ext2_context_t));
    return true;
  }
  return false;
}
