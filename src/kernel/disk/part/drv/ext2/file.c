// file.c
// ----------------------
// Logical-to-physical block mapping and regular file data I/O.
//
// ext2 uses twelve direct block pointers followed by single, double and triple
// indirect blocks.  The mapper below walks those levels iteratively so it only
// needs one scratch block even while allocating missing indirect blocks.
#include "ext2.h"
#include "helpers/string.h"

static uint32_t ext2_bmap_alloc(ext2_context_t *ctx, ext2_inode_t *inode,
                                bool_t *inode_dirty) {
  uint32_t block = ext2_alloc_block(ctx);

  if (block != 0) {
    inode->num_sectors += ctx->sectors_per_block;
    *inode_dirty = true;
  }
  return block;
}

// level is 1 (single indirect), 2 (double) or 3 (triple).  *top points at the
// inode field (or an earlier indirect entry) that owns this subtree.
static int ext2_bmap_indirect(ext2_context_t *ctx, ext2_inode_t *inode,
                              uint32_t *top, uint32_t logical, uint32_t level,
                              bool_t create, uint32_t *out,
                              bool_t *inode_dirty) {
  uint32_t current_block;
  uint32_t l;

  *out = 0;

  if (*top == 0) {
    if (!create)
      return 0;
    current_block = ext2_bmap_alloc(ctx, inode, inode_dirty);
    if (current_block == 0)
      return ENOSPC;
    *top = current_block;
  }

  current_block = *top;
  for (l = level; l >= 1; --l) {
    uint64_t divisor = 1;
    uint32_t index;
    uint32_t *entries;
    uint32_t next;
    uint32_t i;
    int rc;

    for (i = 1; i < l; ++i)
      divisor *= ctx->ptrs_per_block;

    index = (uint32_t)(logical / divisor);
    logical = (uint32_t)(logical % divisor);

    rc = ext2_read_block(ctx, current_block, ctx->block_buf);
    if (rc != 0)
      return rc;
    entries = (uint32_t *)ctx->block_buf;

    next = entries[index];
    if (l == 1) {
      if (next == 0) {
        if (!create)
          return 0;
        next = ext2_bmap_alloc(ctx, inode, inode_dirty);
        if (next == 0)
          return ENOSPC;
        entries[index] = next;
        rc = ext2_write_block(ctx, current_block, ctx->block_buf);
        if (rc != 0)
          return rc;
      }
      *out = next;
      return 0;
    }

    if (next == 0) {
      if (!create)
        return 0;
      next = ext2_bmap_alloc(ctx, inode, inode_dirty);
      if (next == 0)
        return ENOSPC;
      entries[index] = next;
      rc = ext2_write_block(ctx, current_block, ctx->block_buf);
      if (rc != 0)
        return rc;
    }

    current_block = next;
  }

  return 0;
}

int ext2_bmap(ext2_context_t *ctx, ext2_inode_t *inode, uint32_t logical,
              bool_t create, uint32_t *phys, bool_t *inode_dirty) {
  uint32_t *direct;
  uint32_t ptrs;
  uint64_t double_span;

  if (ctx == NULL || inode == NULL || phys == NULL || inode_dirty == NULL)
    return EINVAL;

  direct = &inode->dbp0;
  ptrs = ctx->ptrs_per_block;
  double_span = (uint64_t)ptrs * ptrs;

  *phys = 0;

  if (logical < EXT2_NDIR_BLOCKS) {
    uint32_t block = direct[logical];
    if (block == 0 && create) {
      block = ext2_bmap_alloc(ctx, inode, inode_dirty);
      if (block == 0)
        return ENOSPC;
      direct[logical] = block;
    }
    *phys = block;
    return 0;
  }
  logical -= EXT2_NDIR_BLOCKS;

  if (logical < ptrs)
    return ext2_bmap_indirect(ctx, inode, &inode->sibp, logical, 1, create,
                              phys, inode_dirty);

  logical -= ptrs;
  if ((uint64_t)logical < double_span)
    return ext2_bmap_indirect(ctx, inode, &inode->dibp, logical, 2, create,
                              phys, inode_dirty);

  logical -= (uint32_t)double_span;
  if ((uint64_t)logical < double_span * ptrs)
    return ext2_bmap_indirect(ctx, inode, &inode->tibp, logical, 3, create,
                              phys, inode_dirty);

  return EINVAL;
}

int ext2_read_file(ext2_context_t *ctx, ext2_inode_t *inode, uint64_t offset,
                   void *buf, uint32_t count) {
  uint64_t size;
  uint32_t done = 0;
  uint8_t *dst = (uint8_t *)buf;

  if (ctx == NULL || inode == NULL || buf == NULL)
    return EINVAL;

  size = ext2_inode_size(inode);
  if (offset >= size || count == 0)
    return 0;
  if ((uint64_t)count > size - offset)
    count = (uint32_t)(size - offset);

  while (done < count) {
    uint64_t logical64 = offset / ctx->block_size;
    uint32_t within = (uint32_t)(offset % ctx->block_size);
    uint32_t chunk = ctx->block_size - within;
    uint32_t phys = 0;
    bool_t dirty = false;
    int rc;

    if (chunk > count - done)
      chunk = count - done;
    if (logical64 > 0xFFFFFFFFull)
      return EINVAL;

    rc = ext2_bmap(ctx, inode, (uint32_t)logical64, false, &phys, &dirty);
    if (rc != 0)
      return rc;

    if (phys == 0) {
      uint32_t i;
      for (i = 0; i < chunk; ++i)
        dst[done + i] = 0;
    } else if (within == 0 && chunk == ctx->block_size) {
      rc = ext2_read_block(ctx, phys, dst + done);
      if (rc != 0)
        return rc;
    } else {
      rc = ext2_read_block(ctx, phys, ctx->block_buf);
      if (rc != 0)
        return rc;
      memcpy(dst + done, ctx->block_buf + within, chunk);
    }

    offset += chunk;
    done += chunk;
  }

  return (int)done;
}

int ext2_write_file(ext2_context_t *ctx, ext2_inode_t *inode, uint64_t offset,
                    const void *buf, uint32_t count, bool_t *inode_dirty) {
  uint64_t old_size;
  uint64_t end;
  uint64_t new_size;
  uint32_t done = 0;
  const uint8_t *src = (const uint8_t *)buf;
  int last_error = 0;

  if (ctx == NULL || inode == NULL || buf == NULL || inode_dirty == NULL)
    return EINVAL;
  if (count == 0)
    return 0;

  if (inode->flags & EXT2_INODE_FLAG_IMMUTABLE)
    return EPERM;

  old_size = ext2_inode_size(inode);
  if (inode->flags & EXT2_INODE_FLAG_APPEND) {
    if (offset != old_size)
      return EPERM;
  }

  end = offset + (uint64_t)count;
  if (end < offset)
    return EINVAL;

  if (end > 0xFFFFFFFFull &&
      (ctx->superblock->main.version_major < 1 ||
       (ctx->superblock->extend.ro_compat_features &
        EXT2_FEATURE_RO_COMPAT_LARGE_FILE) == 0))
    return EINVAL;

  while (done < count) {
    uint64_t logical64 = offset / ctx->block_size;
    uint32_t within = (uint32_t)(offset % ctx->block_size);
    uint32_t chunk = ctx->block_size - within;
    uint32_t phys = 0;
    int rc;

    if (chunk > count - done)
      chunk = count - done;
    if (logical64 > 0xFFFFFFFFull) {
      last_error = EINVAL;
      break;
    }

    rc = ext2_bmap(ctx, inode, (uint32_t)logical64, true, &phys, inode_dirty);
    if (rc != 0) {
      last_error = rc;
      break;
    }
    if (phys == 0) {
      last_error = ENOSPC;
      break;
    }

    if (within == 0 && chunk == ctx->block_size) {
      memcpy(ctx->block_buf, src + done, chunk);
    } else {
      rc = ext2_read_block(ctx, phys, ctx->block_buf);
      if (rc != 0) {
        last_error = rc;
        break;
      }
      memcpy(ctx->block_buf + within, src + done, chunk);
    }

    rc = ext2_write_block(ctx, phys, ctx->block_buf);
    if (rc != 0) {
      last_error = rc;
      break;
    }

    offset += chunk;
    done += chunk;
  }

  new_size = offset > old_size ? offset : old_size;
  if (new_size != old_size || done != 0) {
    inode->size_lower = (uint32_t)new_size;
    inode->size_upper = (uint32_t)(new_size >> 32);
    inode->modify_time = (uint32_t)ext2_now();
    inode->create_time = (uint32_t)ext2_now();
    *inode_dirty = true;
  }

  if (done == 0 && last_error != 0)
    return last_error;
  return (int)done;
}
