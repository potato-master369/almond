// inode.c
// ----------------------
// Inode loading, writing and path resolution.
//
// These are internal functions; not meant to face the rest of the kernel
// directly.  ext2.c provides the partition-level callbacks.
#include "ext2.h"
#include "helpers/string.h"
#include "messaging/messaging.h"
#include "mmu/kmalloc.h"

int ext2_read_inode(ext2_context_t *ctx, uint32_t inode_n, ext2_inode_t *out) {
  ext2_bgd_t bgd;
  uint32_t group;
  uint32_t index;
  uint64_t byte_offset;
  uint32_t block_offset;
  uint32_t offset_in_block;
  uint32_t copy_size;
  int rc;

  if (ctx == NULL || out == NULL || inode_n == 0 ||
      inode_n > ctx->superblock->main.num_inodes)
    return EINVAL;

  group = (inode_n - 1) / ctx->inodes_per_group;
  index = (inode_n - 1) % ctx->inodes_per_group;
  if (group >= ctx->group_count)
    return EIO;

  rc = ext2_read_bgd(ctx, group, &bgd);
  if (rc != 0)
    return rc;

  byte_offset = (uint64_t)index * (uint64_t)ctx->inode_size;
  block_offset = (uint32_t)(byte_offset / ctx->block_size);
  offset_in_block = (uint32_t)(byte_offset % ctx->block_size);

  rc = ext2_read_block(ctx, bgd.inode_table_starting_addr + block_offset,
                       ctx->block_buf);
  if (rc != 0)
    return rc;

  copy_size = ctx->inode_size;
  if (copy_size > sizeof(ext2_inode_t))
    copy_size = sizeof(ext2_inode_t);
  memcpy(out, ctx->block_buf + offset_in_block, copy_size);
  if (copy_size < sizeof(ext2_inode_t)) {
    uint8_t *tail = (uint8_t *)out + copy_size;
    uint32_t rest = sizeof(ext2_inode_t) - copy_size;
    while (rest--)
      *tail++ = 0;
  }
  return 0;
}

int ext2_write_inode(ext2_context_t *ctx, uint32_t inode_n,
                     const ext2_inode_t *in) {
  ext2_bgd_t bgd;
  uint32_t group;
  uint32_t index;
  uint64_t byte_offset;
  uint32_t block_offset;
  uint32_t offset_in_block;
  uint32_t copy_size;
  int rc;

  if (ctx == NULL || in == NULL || inode_n == 0 ||
      inode_n > ctx->superblock->main.num_inodes)
    return EINVAL;

  group = (inode_n - 1) / ctx->inodes_per_group;
  index = (inode_n - 1) % ctx->inodes_per_group;
  if (group >= ctx->group_count)
    return EIO;

  rc = ext2_read_bgd(ctx, group, &bgd);
  if (rc != 0)
    return rc;

  byte_offset = (uint64_t)index * (uint64_t)ctx->inode_size;
  block_offset = (uint32_t)(byte_offset / ctx->block_size);
  offset_in_block = (uint32_t)(byte_offset % ctx->block_size);

  rc = ext2_read_block(ctx, bgd.inode_table_starting_addr + block_offset,
                       ctx->block_buf);
  if (rc != 0)
    return rc;

  copy_size = ctx->inode_size;
  if (copy_size > sizeof(ext2_inode_t))
    copy_size = sizeof(ext2_inode_t);
  memcpy(ctx->block_buf + offset_in_block, in, copy_size);

  return ext2_write_block(ctx, bgd.inode_table_starting_addr + block_offset,
                          ctx->block_buf);
}

uint64_t ext2_inode_size(const ext2_inode_t *inode) {
  uint64_t size;

  if (inode == NULL)
    return 0;
  size = inode->size_lower;
  if (ext2_inode_type(inode) == EXT2_S_IFREG)
    size |= ((uint64_t)inode->size_upper) << 32;
  return size;
}

uint16_t ext2_inode_type(const ext2_inode_t *inode) {
  if (inode == NULL)
    return 0;
  return inode->permissions & EXT2_S_IFMT;
}

// Read a symbolic link target.  Classic ext2 stores short targets inside the
// inode's block pointer area and longer ones in normal data blocks.
static int ext2_readlink(ext2_context_t *ctx, const ext2_inode_t *inode,
                         char **out, uint32_t *out_len) {
  uint64_t size64;
  uint32_t size;
  char *buf;
  int rc;

  size64 = ext2_inode_size(inode);
  if (size64 > 4096)
    return EINVAL; // an ext2 symlink target longer than this is nonsense
  size = (uint32_t)size64;

  buf = (char *)kmalloc(size + 1);
  if (buf == NULL)
    return ENOMEM;

  if (inode->num_sectors == 0 && size <= EXT2_MAX_SYMLINK_LEN) {
    memcpy(buf, &inode->dbp0, size);
    rc = 0;
  } else {
    rc = ext2_read_file(ctx, (ext2_inode_t *)inode, 0, buf, size);
    if (rc < 0) {
      kfree(buf);
      return rc;
    }
  }
  buf[size] = '\0';
  *out = buf;
  *out_len = size;
  return 0;
}

// Resolve a path relative to start_ino.  Absolute paths always restart at the
// root inode.  Symbolic links are expanded iteratively with a depth limit.
static int ext2_resolve_from(ext2_context_t *ctx, uint32_t start_ino,
                             const char *path, int depth, uint32_t *out_ino,
                             ext2_inode_t *out_inode) {
  ext2_inode_t current;
  uint32_t current_ino;
  const char *p;
  int rc;

  if (ctx == NULL || path == NULL || out_ino == NULL || out_inode == NULL)
    return EINVAL;
  if (depth > EXT2_MAX_SYMLINK_DEPTH)
    return EINVAL;

  p = path;
  if (*p == '/') {
    current_ino = EXT2_ROOT_INO;
    while (*p == '/')
      ++p;
  } else {
    current_ino = start_ino;
  }

  rc = ext2_read_inode(ctx, current_ino, &current);
  if (rc != 0)
    return rc;

  while (*p != '\0') {
    const char *name_start;
    const char *rest;
    char name[256];
    uint32_t name_len;
    uint32_t next_ino;
    ext2_inode_t next;

    while (*p == '/')
      ++p;
    if (*p == '\0')
      break;

    if (ext2_inode_type(&current) != EXT2_S_IFDIR)
      return EINVAL;

    name_start = p;
    while (*p != '\0' && *p != '/')
      ++p;
    name_len = (uint32_t)(p - name_start);
    if (name_len == 0 || name_len > 255)
      return ENOENT;
    memcpy(name, name_start, name_len);
    name[name_len] = '\0';

    rest = p;
    while (*rest == '/')
      ++rest;

    if (name_len == 1 && name[0] == '.') {
      next_ino = current_ino;
    } else if (name_len == 2 && name[0] == '.' && name[1] == '.') {
      if (current_ino == EXT2_ROOT_INO) {
        next_ino = EXT2_ROOT_INO;
      } else if (ext2_dir_lookup(ctx, &current, "..", 2, &next_ino) != 0) {
        next_ino = EXT2_ROOT_INO;
      }
    } else {
      rc = ext2_dir_lookup(ctx, &current, name, (uint8_t)name_len, &next_ino);
      if (rc != 0)
        return rc;
    }

    rc = ext2_read_inode(ctx, next_ino, &next);
    if (rc != 0)
      return rc;

    if (ext2_inode_type(&next) == EXT2_S_IFLNK) {
      char *link;
      char *combined;
      uint32_t link_len;
      uint32_t rest_len;
      uint32_t total_len;
      uint32_t base_ino;
      bool_t absolute;

      rc = ext2_readlink(ctx, &next, &link, &link_len);
      if (rc != 0)
        return rc;
      if (link_len == 0) {
        kfree(link);
        return ENOENT;
      }

      absolute = (link[0] == '/');
      rest_len = 0;
      while (rest[rest_len] != '\0')
        ++rest_len;
      total_len = link_len + (rest_len != 0 ? (1 + rest_len) : 0);

      combined = (char *)kmalloc(total_len + 1);
      if (combined == NULL) {
        kfree(link);
        return ENOMEM;
      }
      memcpy(combined, link, link_len);
      if (rest_len != 0) {
        combined[link_len] = '/';
        memcpy(combined + link_len + 1, rest, rest_len);
      }
      combined[total_len] = '\0';
      kfree(link);

      base_ino = absolute ? EXT2_ROOT_INO : current_ino;
      rc = ext2_resolve_from(ctx, base_ino, combined, depth + 1, out_ino,
                             out_inode);
      kfree(combined);
      return rc;
    }

    current_ino = next_ino;
    current = next;
    p = rest;
  }

  *out_ino = current_ino;
  *out_inode = current;
  return 0;
}

int ext2_resolve_path(ext2_context_t *ctx, const char *path, uint32_t *out_ino,
                      ext2_inode_t *out_inode) {
  return ext2_resolve_from(ctx, EXT2_ROOT_INO, path, 0, out_ino, out_inode);
}
