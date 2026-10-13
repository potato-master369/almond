// ext2.c
// ---------------------------
// Kernel glue code for ext2
//
// Basically i decide what to steal later
// and then put that ONTO this mechanism
#include "../../../../kernel_types.h"
#include "../../part.h"
#include "../../../disk.h"
#include "ext2.h"
#include "include/def.h"
#include "../../../../messaging/messaging.h"
#include "../../../../mmu/kmalloc.h"

uint32_t ext2_resolve_inode_n(const char *path) {
  return 0;
}

uint32_t ext2_resolve_generation_count(uint32_t inode_n) {
  return 0;
}

uint32_t ext2_resolve_internal_fh_id(uint32_t inode_n) {
  return 0;
}

uint8_t ext2_open_fh(almond_filehandle_t *f, const char *path) {
  f->fsid = FSID_DRV_EXT2;
  f->inode_n = ext2_resolve_inode_n(path);
  f->resv = ext2_resolve_internal_fh_id(f->inode_n);
  f->generation_count = ext2_resolve_generation_count(f->inode_n);
  return 0;
}

uint8_t ext2_close_fh(almond_filehandle_t *f) {
  return 0;
}

uint8_t ext2_read(almond_filehandle_t *fh, void *to, int n) {
  return 0;
}

uint8_t ext2_write(almond_filehandle_t *fh, void *from, int n) {
  return 0;
}

int ext2_mount_part(partition_info_t *target, disk_info_t *parent, uint8_t part_n) {
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
    target->openstream = ext2_open_fh;
    target->closestream = ext2_close_fh;
    target->read = ext2_read;
    target->write = ext2_write;
    kfree(sb);
    return true;
  }
  return false;
}
