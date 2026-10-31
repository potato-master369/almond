// ext2.c
// ---------------------------
// Kernel glue code for ext2
//
// This file owns the partition-level callbacks and the list of mounted ext2
// filesystems.  The heavy lifting lives in block.c, inode.c, dir.c and file.c.
#include "ext2.h"
#include "kernel_types.h"
#include "messaging/messaging.h"
#include "mmu/kmalloc.h"
#include "disk/disk.h"
#include "disk/part/part.h"
#include "include/def.h"
#include "clock/clock.h"

extern int64_t boottime;
extern disk_info_t disk_db[DISK_MAX];

// Only the head of the mounted-filesystem list lives in BSS.  Everything else
// is allocated while mounting.
static ext2_context_t *ext2_mounts;

uint64_t ext2_now(void) {
  int64_t now = boottime + (clock_get_ms_since_boot() / 1000);
  return now < 0 ? 0 : (uint64_t)now;
}

ext2_context_t *ext2_context_from_fsid(uint32_t fsid) {
  ext2_context_t *ctx;

  for (ctx = ext2_mounts; ctx != NULL; ctx = ctx->next) {
    if (ctx->fsid == fsid)
      return ctx;
  }

  // A caller that has not filled in fsid yet may still use the only mount.
  if (fsid == 0 && ext2_mounts != NULL && ext2_mounts->next == NULL)
    return ext2_mounts;
  return NULL;
}

static uint32_t ext2_disk_index(disk_info_t *parent) {
  uint32_t i;

  if (parent == NULL)
    return 0;
  for (i = 0; i < DISK_MAX; ++i) {
    if (&disk_db[i] == parent)
      return i;
  }
  return 0;
}

static bool_t ext2_is_power_of_two(uint32_t value) {
  return value != 0 && (value & (value - 1)) == 0;
}

static void ext2_free_context(ext2_context_t *ctx) {
  if (ctx == NULL)
    return;
  if (ctx->superblock != NULL)
    kfree(ctx->superblock);
  if (ctx->block_group_desc != NULL)
    kfree(ctx->block_group_desc);
  if (ctx->block_buf != NULL)
    kfree(ctx->block_buf);
  if (ctx->aux_buf != NULL)
    kfree(ctx->aux_buf);
  kfree(ctx);
}

static void ext2_unlink_context(ext2_context_t *ctx) {
  ext2_context_t **link = &ext2_mounts;

  while (*link != NULL) {
    if (*link == ctx) {
      *link = ctx->next;
      return;
    }
    link = &(*link)->next;
  }
}

int ext2_mount_part(partition_info_t *target, disk_info_t *parent,
                    uint8_t part_n) {
  ext2_superblock_full_t *sb;
  ext2_context_t *ctx;
  uint32_t total_blocks;
  uint32_t first_data;
  uint32_t blocks_per_group;
  uint32_t inodes_per_group;
  uint32_t block_size;
  uint32_t inode_size;
  uint32_t group_count;
  uint32_t incompat;
  uint32_t rocompat;
  uint64_t fs_size;

  if (target == NULL || parent == NULL || parent->read == NULL)
    return false;
  if (part_n >= 4)
    return false;

  sb = (ext2_superblock_full_t *)kmalloc(sizeof(ext2_superblock_full_t));
  if (sb == NULL) {
    message_send_message(" ext2: mount kmalloc fail!\n");
    return false;
  }

  if (parent->read(parent, parent->partition_metadata[part_n].lba_start + 2, 2,
                   sb) != 0) {
    message_send_message(" ext2: superblock read fail!\n");
    kfree(sb);
    return false;
  }

  if (sb->main.sig != EXT2_MAGIC) {
    kfree(sb);
    return false;
  }

  if (sb->main.log2_block_size > 2) {
    message_send_message(" ext2: unsupported block size\n");
    kfree(sb);
    return false;
  }
  block_size = EXT2_MIN_BLOCK_SIZE << sb->main.log2_block_size;
  if (block_size < EXT2_MIN_BLOCK_SIZE || block_size > EXT2_MAX_BLOCK_SIZE) {
    message_send_message(" ext2: bad block size\n");
    kfree(sb);
    return false;
  }

  if (sb->main.version_major > 1) {
    message_send_message(" ext2: unsupported revision\n");
    kfree(sb);
    return false;
  }

  // Classic ext2 only.  Reject anything that needs journal replay, extents,
  // 64-bit block numbers, compression, etc.  Compatible bits are normally
  // safe to ignore, but has_journal means this is an ext3/ext4 filesystem:
  // its metadata may depend on a journal we cannot replay, so refuse it.
  if (sb->main.version_major >= 1) {
    if (sb->extend.optional_features & EXT2_FEATURE_COMPAT_HAS_JOURNAL) {
      message_send_message(" ext2: journaling filesystem, not supported\n");
      kfree(sb);
      return false;
    }
    incompat = sb->extend.required_features &
               ~((uint32_t)EXT2_FEATURE_INCOMPAT_FILETYPE);
    if (incompat != 0) {
      message_send_message(" ext2: unsupported incompatible features\n");
      kfree(sb);
      return false;
    }
    rocompat = sb->extend.ro_compat_features &
               ~((uint32_t)(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER |
                            EXT2_FEATURE_RO_COMPAT_LARGE_FILE));
    if (rocompat != 0) {
      message_send_message(" ext2: unsupported read-only features\n");
      kfree(sb);
      return false;
    }
    inode_size = sb->extend.sizeof_inode;
  } else {
    inode_size = EXT2_MIN_INODE_SIZE;
  }

  if (inode_size < EXT2_MIN_INODE_SIZE || inode_size > block_size ||
      !ext2_is_power_of_two(inode_size)) {
    message_send_message(" ext2: bad inode size\n");
    kfree(sb);
    return false;
  }

  total_blocks = sb->main.total_blocks;
  first_data = sb->main.superblock_block_no;
  blocks_per_group = sb->main.block_group_num_groups;
  inodes_per_group = sb->main.block_group_num_inodes;

  if (total_blocks == 0 || blocks_per_group == 0 || inodes_per_group == 0 ||
      first_data >= total_blocks) {
    message_send_message(" ext2: invalid superblock geometry\n");
    kfree(sb);
    return false;
  }

  group_count =
      (total_blocks - first_data + blocks_per_group - 1) / blocks_per_group;
  if (group_count == 0) {
    message_send_message(" ext2: no block groups\n");
    kfree(sb);
    return false;
  }

  ctx = (ext2_context_t *)kmalloc(sizeof(ext2_context_t));
  if (ctx == NULL) {
    message_send_message(" ext2: context kmalloc fail!\n");
    kfree(sb);
    return false;
  }

  ctx->superblock = sb;
  ctx->block_group_desc = (ext2_bgd_t *)kmalloc(sizeof(ext2_bgd_t));
  ctx->parent = parent;
  ctx->next = NULL;
  ctx->block_buf = (uint8_t *)kmalloc(block_size);
  ctx->aux_buf = (uint8_t *)kmalloc(block_size);
  if (ctx->block_group_desc == NULL || ctx->block_buf == NULL ||
      ctx->aux_buf == NULL) {
    message_send_message(" ext2: buffer kmalloc fail!\n");
    ext2_free_context(ctx);
    return false;
  }

  ctx->fsid = EXT2_FSID(ext2_disk_index(parent), part_n);
  ctx->lba_start = parent->partition_metadata[part_n].lba_start;
  ctx->part_n = part_n;
  ctx->block_size = block_size;
  ctx->sectors_per_block = block_size / SECTOR_SIZE;
  ctx->inode_size = inode_size;
  ctx->inodes_per_group = inodes_per_group;
  ctx->blocks_per_group = blocks_per_group;
  ctx->group_count = group_count;
  ctx->gdt_block = (block_size == 1024) ? 2 : 1;
  ctx->gdt_entries_per_block = block_size / sizeof(ext2_bgd_t);
  ctx->ptrs_per_block = block_size / sizeof(uint32_t);
  ctx->first_data_block = first_data;
  ctx->inodes_per_block = block_size / inode_size;
  ctx->inode_blocks_per_group =
      (inodes_per_group + ctx->inodes_per_block - 1) / ctx->inodes_per_block;
  ctx->cached_group = -1;
  ctx->super_dirty = false;

  if (ctx->gdt_block >= total_blocks || ctx->gdt_entries_per_block == 0 ||
      ctx->inodes_per_block == 0) {
    message_send_message(" ext2: invalid group table\n");
    ext2_free_context(ctx);
    return false;
  }

  // Remember when we mounted and bump the mount counter.  As in the original
  // driver this is kept in the in-memory copy; the actual superblock is only
  // written back when ext2 itself changes filesystem metadata.
  sb->main.last_mount_time = (uint32_t)ext2_now();
  if (sb->main.num_post_fsck_mounts != 0xFFFF)
    ++sb->main.num_post_fsck_mounts;

  ctx->next = ext2_mounts;
  ext2_mounts = ctx;

  target->parent = parent;
  target->read = ext2_read;
  target->write = ext2_write;
  target->openstream = ext2_openstream;
  target->closestream = ext2_closestream;
  target->stat = ext2_stat;
  target->dir_index = ext2_dir_index;
  target->dir_count = ext2_dir_count;
  target->resv = (uint32_t)ctx;
  target->part_n = part_n;
  target->present = true;

  fs_size = (uint64_t)total_blocks * block_size;
  target->size = fs_size > 0xFFFFFFFFull ? 0xFFFFFFFFu : (uint32_t)fs_size;

  message_send_message(" ext2: found!\n");
  return true;
}

int ext2_umount_part(partition_info_t *target, uint8_t part_n) {
  ext2_context_t *ctx;

  (void)part_n;
  if (target == NULL)
    return EINVAL;

  ctx = (ext2_context_t *)target->resv;
  if (ctx == NULL)
    return 0;

  (void)ext2_flush_super(ctx);
  ext2_unlink_context(ctx);
  ext2_free_context(ctx);

  target->resv = 0;
  target->read = NULL;
  target->write = NULL;
  target->openstream = NULL;
  target->closestream = NULL;
  target->stat = NULL;
  target->dir_index = NULL;
  target->dir_count = NULL;
  target->present = false;
  return 0;
}

static ext2_file_t *ext2_file_from_handle(almond_filehandle_t *fh) {
  if (fh == NULL || fh->resv == 0)
    return NULL;
  return (ext2_file_t *)fh->resv;
}

int ext2_openstream(almond_filehandle_t *fh, const char *filename) {
  ext2_context_t *ctx;
  ext2_file_t *file;
  ext2_inode_t inode;
  uint32_t inode_n;
  int rc;

  if (fh == NULL || filename == NULL)
    return EINVAL;

  ctx = ext2_context_from_fsid(fh->fsid);
  if (ctx == NULL)
    return ENOENT;

  rc = ext2_resolve_path(ctx, filename, &inode_n, &inode);
  if (rc != 0)
    return rc;

  file = (ext2_file_t *)kmalloc(sizeof(ext2_file_t));
  if (file == NULL)
    return ENOMEM;

  file->fs = ctx;
  file->inode = inode;
  file->inode_n = inode_n;
  file->cursor = 0;
  file->flags = 0;

  fh->resv = (uint32_t)file;
  fh->inode_n = inode_n;
  fh->pos = 0;
  return 0;
}

int ext2_closestream(almond_filehandle_t *fh) {
  ext2_file_t *file;

  if (fh == NULL)
    return EINVAL;

  file = ext2_file_from_handle(fh);
  if (file != NULL) {
    kfree(file);
    fh->resv = 0;
  }
  return 0;
}

int ext2_read(almond_filehandle_t *fh, void *to, int n) {
  ext2_file_t *file;
  uint16_t type;
  int rc;

  file = ext2_file_from_handle(fh);
  if (file == NULL || to == NULL)
    return EINVAL;
  if (n <= 0)
    return 0;

  type = ext2_inode_type(&file->inode);
  if (type == EXT2_S_IFDIR)
    return EISDIR;
  if (type != EXT2_S_IFREG)
    return EINVAL;

  // The cursor is kept in the private file context.  fh->pos is only a
  // mirror; if somebody changed it we honour that as the new cursor.
  if (fh->pos != file->cursor)
    file->cursor = fh->pos;

  rc = ext2_read_file(file->fs, &file->inode, file->cursor, to, (uint32_t)n);
  if (rc < 0)
    return rc;

  file->cursor += (uint32_t)rc;
  fh->pos = file->cursor;
  fh->inode_n = file->inode_n;
  return rc;
}

int ext2_write(almond_filehandle_t *fh, void *from, int n) {
  ext2_file_t *file;
  uint16_t type;
  bool_t inode_dirty = false;
  int rc;

  file = ext2_file_from_handle(fh);
  if (file == NULL || from == NULL)
    return EINVAL;
  if (n <= 0)
    return 0;

  type = ext2_inode_type(&file->inode);
  if (type == EXT2_S_IFDIR)
    return EISDIR;
  if (type != EXT2_S_IFREG)
    return EINVAL;

  if (fh->pos != file->cursor)
    file->cursor = fh->pos;

  rc = ext2_write_file(file->fs, &file->inode, file->cursor, from,
                       (uint32_t)n, &inode_dirty);
  if (rc < 0)
    return rc;

  file->fs->superblock->main.last_write_time = (uint32_t)ext2_now();
  file->fs->super_dirty = true;

  if (inode_dirty) {
    int wrc = ext2_write_inode(file->fs, file->inode_n, &file->inode);
    if (wrc != 0)
      return wrc;
  }
  if (file->fs->super_dirty) {
    int src = ext2_flush_super(file->fs);
    if (src != 0)
      return src;
  }

  file->cursor += (uint32_t)rc;
  fh->pos = file->cursor;
  fh->inode_n = file->inode_n;
  return rc;
}

int ext2_stat(almond_stat_t *out, const char *path) {
  ext2_context_t *ctx;
  ext2_inode_t inode;
  uint64_t size;
  uint16_t type;
  uint32_t inode_n;
  uint32_t fsid;
  int rc;

  if (out == NULL || path == NULL)
    return EINVAL;

  fsid = out->fsid;
  ctx = ext2_context_from_fsid(fsid);
  if (ctx == NULL)
    return ENOENT;

  rc = ext2_resolve_path(ctx, path, &inode_n, &inode);
  if (rc != 0)
    return rc;

  type = ext2_inode_type(&inode);
  if (type == EXT2_S_IFREG)
    out->type = A_FILE;
  else if (type == EXT2_S_IFDIR)
    out->type = A_DIR;
  else
    out->type = A_OTHER;

  out->r = (inode.permissions & EXT2_S_IRUSR) != 0;
  out->w = (inode.permissions & EXT2_S_IWUSR) != 0;
  out->x = (inode.permissions & EXT2_S_IXUSR) != 0;

  size = ext2_inode_size(&inode);
  out->size = size > 0xFFFFFFFFull ? 0xFFFFFFFFu : (uint32_t)size;
  out->inode_n = inode_n;
  out->fsid = ctx->fsid;
  return 0;
}

static int ext2_handle_dir(almond_filehandle_t *fh, ext2_context_t **out_ctx,
                           ext2_inode_t **out_inode) {
  ext2_file_t *file;

  if (fh == NULL)
    return EINVAL;

  file = ext2_file_from_handle(fh);
  if (file != NULL) {
    *out_ctx = file->fs;
    *out_inode = &file->inode;
    return 0;
  }

  *out_ctx = ext2_context_from_fsid(fh->fsid);
  if (*out_ctx == NULL || fh->inode_n == 0)
    return ENOENT;
  return 1; // caller still needs to load the inode
}

int ext2_dir_index(almond_filehandle_t *dir, uint32_t index,
                   almond_dirent_t *out) {
  ext2_context_t *ctx;
  ext2_inode_t *inode;
  ext2_inode_t temp;
  int rc;

  rc = ext2_handle_dir(dir, &ctx, &inode);
  if (rc == 1) {
    rc = ext2_read_inode(ctx, dir->inode_n, &temp);
    if (rc != 0)
      return rc;
    inode = &temp;
  } else if (rc != 0) {
    return rc;
  }

  return ext2_dir_index_inode(ctx, inode, index, out);
}

int ext2_dir_count(almond_filehandle_t *dir, uint32_t *out) {
  ext2_context_t *ctx;
  ext2_inode_t *inode;
  ext2_inode_t temp;
  int rc;

  rc = ext2_handle_dir(dir, &ctx, &inode);
  if (rc == 1) {
    rc = ext2_read_inode(ctx, dir->inode_n, &temp);
    if (rc != 0)
      return rc;
    inode = &temp;
  } else if (rc != 0) {
    return rc;
  }

  return ext2_dir_count_inode(ctx, inode, out);
}
