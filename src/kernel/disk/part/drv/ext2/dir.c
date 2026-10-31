// dir.c
// ----------------------
// ext2 directory iteration, lookup and enumeration.
//
// Directories are just files containing variable-length ext2_dirent_t
// records.  A record never crosses a filesystem block boundary.
#include "ext2.h"
#include "helpers/string.h"

typedef int (*ext2_dir_callback_t)(void *arg, const char *name,
                                   uint32_t name_len, uint32_t inode,
                                   uint8_t type);

// Returns 0 when the whole directory was scanned, 1 when the callback stopped
// the scan, or a negative error code.
static int ext2_dir_iterate(ext2_context_t *ctx, const ext2_inode_t *dir,
                            ext2_dir_callback_t callback, void *arg) {
  uint64_t size;
  uint32_t block_size;
  uint32_t block_count;
  uint32_t logical;

  if (ctx == NULL || dir == NULL || callback == NULL)
    return EINVAL;
  if (ext2_inode_type(dir) != EXT2_S_IFDIR)
    return EINVAL;

  size = ext2_inode_size(dir);
  block_size = ctx->block_size;
  block_count = (uint32_t)((size + block_size - 1) / block_size);

  for (logical = 0; logical < block_count; ++logical) {
    bool_t inode_dirty = false;
    uint32_t phys = 0;
    uint32_t limit;
    uint32_t off;
    uint64_t block_base;
    int rc;

    rc = ext2_bmap(ctx, (ext2_inode_t *)dir, logical, false, &phys,
                   &inode_dirty);
    if (rc != 0)
      return rc;
    if (phys == 0)
      continue; // sparse directory block; nothing to read

    rc = ext2_read_block(ctx, phys, ctx->block_buf);
    if (rc != 0)
      return rc;

    block_base = (uint64_t)logical * block_size;
    limit = block_size;
    if (size - block_base < limit)
      limit = (uint32_t)(size - block_base);

    off = 0;
    while (off + sizeof(ext2_dirent_t) <= limit) {
      ext2_dirent_t *de = (ext2_dirent_t *)(ctx->block_buf + off);
      uint32_t rec_len = de->size;
      uint32_t name_len = de->name_length;

      if (rec_len < sizeof(ext2_dirent_t) || (rec_len & 3u) != 0 ||
          off + rec_len > limit)
        break; // malformed directory block
      if (name_len > rec_len - sizeof(ext2_dirent_t))
        break;

      if (de->inode != 0 && name_len != 0) {
        rc = callback(arg, de->name, name_len, de->inode,
                      de->type_indicator);
        if (rc < 0)
          return rc;
        if (rc > 0)
          return 1;
      }

      off += rec_len;
    }
  }

  return 0;
}

struct ext2_lookup_arg {
  const char *name;
  uint32_t name_len;
  uint32_t inode;
};

static int ext2_lookup_cb(void *arg, const char *name, uint32_t name_len,
                          uint32_t inode, uint8_t type) {
  struct ext2_lookup_arg *lookup = (struct ext2_lookup_arg *)arg;
  uint32_t i;

  (void)type;
  if (name_len != lookup->name_len)
    return 0;
  for (i = 0; i < name_len; ++i) {
    if (name[i] != lookup->name[i])
      return 0;
  }
  lookup->inode = inode;
  return 1;
}

int ext2_dir_lookup(ext2_context_t *ctx, const ext2_inode_t *dir,
                    const char *name, uint8_t name_len, uint32_t *out_ino) {
  struct ext2_lookup_arg lookup;
  int rc;

  if (name == NULL || out_ino == NULL)
    return EINVAL;

  lookup.name = name;
  lookup.name_len = name_len;
  lookup.inode = 0;

  rc = ext2_dir_iterate(ctx, dir, ext2_lookup_cb, &lookup);
  if (rc < 0)
    return rc;
  if (lookup.inode == 0)
    return ENOENT;

  *out_ino = lookup.inode;
  return 0;
}

struct ext2_count_arg {
  uint32_t count;
};

static int ext2_count_cb(void *arg, const char *name, uint32_t name_len,
                         uint32_t inode, uint8_t type) {
  struct ext2_count_arg *count = (struct ext2_count_arg *)arg;

  (void)inode;
  (void)type;
  // Hide the synthetic "." and ".." entries from directory listings.
  if (name_len == 1 && name[0] == '.')
    return 0;
  if (name_len == 2 && name[0] == '.' && name[1] == '.')
    return 0;
  ++count->count;
  return 0;
}

int ext2_dir_count_inode(ext2_context_t *ctx, const ext2_inode_t *dir,
                         uint32_t *out) {
  struct ext2_count_arg count;
  int rc;

  if (out == NULL)
    return EINVAL;

  count.count = 0;
  rc = ext2_dir_iterate(ctx, dir, ext2_count_cb, &count);
  if (rc < 0)
    return rc;
  *out = count.count;
  return 0;
}

struct ext2_index_arg {
  uint32_t target;
  uint32_t seen;
  almond_dirent_t *out;
  bool_t found;
};

static int ext2_index_cb(void *arg, const char *name, uint32_t name_len,
                         uint32_t inode, uint8_t type) {
  struct ext2_index_arg *index = (struct ext2_index_arg *)arg;
  uint32_t i;

  (void)inode;
  (void)type;
  if (name_len == 1 && name[0] == '.')
    return 0;
  if (name_len == 2 && name[0] == '.' && name[1] == '.')
    return 0;

  if (index->seen != index->target) {
    ++index->seen;
    return 0;
  }

  if (name_len > 255)
    name_len = 255;
  for (i = 0; i < name_len; ++i)
    index->out->name[i] = name[i];
  index->out->name[name_len] = '\0';
  index->found = true;
  return 1;
}

int ext2_dir_index_inode(ext2_context_t *ctx, const ext2_inode_t *dir,
                         uint32_t index, almond_dirent_t *out) {
  struct ext2_index_arg arg;
  int rc;

  if (out == NULL)
    return EINVAL;

  arg.target = index;
  arg.seen = 0;
  arg.out = out;
  arg.found = false;
  out->fsid = ctx->fsid;
  out->name[0] = '\0';

  rc = ext2_dir_iterate(ctx, dir, ext2_index_cb, &arg);
  if (rc < 0)
    return rc;
  return arg.found ? 0 : ENOENT;
}
