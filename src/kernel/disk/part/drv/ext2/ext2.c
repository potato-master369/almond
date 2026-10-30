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
#include "clock/clock.h"

extern int64_t boottime;

static inline long long ipow(int base, int exp) {
    // 0^0 is mathematically undefined, but standard computing practice returns 1
    if (exp == 0) return 1;
    if (exp < 0) {
        // Integer division yields 0 for any base absolute value > 1 when exp is negative
        if (base == 1) return 1;
        if (base == -1) return (exp % 2 == 0) ? 1 : -1;
        return 0; 
    }

    long long result = 1;
    long long current_base = base;

    while (exp > 0) {
        // If the exponent is odd, multiply the current base with the result
        if (exp & 1) { 
            result *= current_base;
        }
        // Square the base and halve the exponent (using bitwise shift)
        current_base *= current_base;
        exp >>= 1; 
    }

    return result;
}
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

// As a small note, the derivation of a partition read
// such as a blk_read_part(disk_n, part_n, whatever)
// is not needed.

// instead drivers shld use
// part->parent->read/write(part->parent, part->parent->partition_metadata[part->part_n].lba_start + offset (reject if offset>= part->parent->partition_metadata[part->part_n].size),  sizeof_rw, buf);
// where,
//  - part is the partition
//  - offset is the 0-indexed offset by sector to read off of the partition
//  - sizeof_rw is the size of the read/write needed.
//  - buf is an allocated buffer.

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
    int64_t epoch = boottime + (clock_get_ms_since_boot() / 1000);
    sb->main.last_mount_time = epoch;
    message_send_message(" ext2: found!\n");
    target->parent = parent;
    target->read = ext2_read;
    target->write = ext2_write;
    target->resv = (uint32_t)kmalloc(sizeof(ext2_context_t));
#define TARGET_CTX ((ext2_context_t*)target->resv)
#define max(a, b) (a > b ? a : b)
    TARGET_CTX->superblock = sb;
    TARGET_CTX->block_group_desc = kmalloc(sizeof(ext2_bgd_t));
    uint64_t offset = max(1024, ipow(2, sb->main.log2_block_size)) + ipow(2, sb->main.log2_block_size);
    parent->read(parent, parent->partition_metadata[part_n].lba_start + offset, sizeof(ext2_bgd_t), TARGET_CTX->block_group_desc);
    TARGET_CTX->parent = parent;
    target->part_n = part_n;
    return true;
  }
  return false;
}

int ext2_umount_part(partition_info_t *target, uint8_t part_n) {
  if (!target->present) // ignore things already unmounted
	  return 0;
  // do things to unmount properly
 
  // free kernel structures
  kfree(TARGET_CTX->superblock);
  kfree(TARGET_CTX->block_group_desc);
  kfree((void*)target->resv);
  return 0;
}
