// block.c
// ---------------------------
// Low-level ext2 block and block-group access.
//
// Blocks are filesystem blocks (1KiB, 2KiB or 4KiB), not disk sectors.  The
// functions below translate them to partition-relative sectors and use the
// parent disk's read/write callbacks.
#include "ext2.h"
#include "helpers/string.h"
#include "messaging/messaging.h"

static void ext2_bzero(void *ptr, uint32_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--)
    *p++ = 0;
}

static int ext2_blk_to_lba(ext2_context_t *ctx, uint32_t block,
                           uint32_t *out_lba) {
  uint64_t lba;

  if (ctx == NULL || out_lba == NULL)
    return EINVAL;
  if (block >= ctx->superblock->main.total_blocks)
    return EIO;

  lba = (uint64_t)ctx->lba_start +
        (uint64_t)block * (uint64_t)ctx->sectors_per_block;
  if (lba + ctx->sectors_per_block > 0xFFFFFFFFull)
    return EIO;
  *out_lba = (uint32_t)lba;
  return 0;
}

int ext2_read_block(ext2_context_t *ctx, uint32_t block, void *buf) {
  uint32_t lba;

  if (ctx == NULL || ctx->parent == NULL || ctx->parent->read == NULL ||
      buf == NULL)
    return EINVAL;
  if (ext2_blk_to_lba(ctx, block, &lba) != 0)
    return EIO;
  if (ctx->parent->read(ctx->parent, lba, ctx->sectors_per_block, buf) != 0)
    return EIO;
  return 0;
}

int ext2_write_block(ext2_context_t *ctx, uint32_t block, const void *buf) {
  uint32_t lba;

  if (ctx == NULL || ctx->parent == NULL || ctx->parent->write == NULL ||
      buf == NULL)
    return EINVAL;
  if (ext2_blk_to_lba(ctx, block, &lba) != 0)
    return EIO;
  if (ctx->parent->write(ctx->parent, lba, ctx->sectors_per_block,
                         (void *)buf) != 0)
    return EIO;
  return 0;
}

int ext2_read_bgd(ext2_context_t *ctx, uint32_t group, ext2_bgd_t *out) {
  uint32_t entries;
  uint32_t block;
  uint32_t index;
  int rc;

  if (ctx == NULL || out == NULL || group >= ctx->group_count)
    return EINVAL;

  if (ctx->cached_group == (int32_t)group) {
    memcpy(out, ctx->block_group_desc, sizeof(*out));
    return 0;
  }

  entries = ctx->gdt_entries_per_block;
  block = ctx->gdt_block + (group / entries);
  index = group % entries;

  rc = ext2_read_block(ctx, block, ctx->aux_buf);
  if (rc != 0)
    return rc;

  memcpy(out, ctx->aux_buf + index * sizeof(ext2_bgd_t), sizeof(*out));
  memcpy(ctx->block_group_desc, out, sizeof(*out));
  ctx->cached_group = (int32_t)group;
  return 0;
}

int ext2_write_bgd(ext2_context_t *ctx, uint32_t group,
                   const ext2_bgd_t *in) {
  uint32_t entries;
  uint32_t block;
  uint32_t index;
  int rc;

  if (ctx == NULL || in == NULL || group >= ctx->group_count)
    return EINVAL;

  entries = ctx->gdt_entries_per_block;
  block = ctx->gdt_block + (group / entries);
  index = group % entries;

  rc = ext2_read_block(ctx, block, ctx->aux_buf);
  if (rc != 0)
    return rc;
  memcpy(ctx->aux_buf + index * sizeof(ext2_bgd_t), in, sizeof(*in));
  rc = ext2_write_block(ctx, block, ctx->aux_buf);
  if (rc != 0)
    return rc;

  memcpy(ctx->block_group_desc, in, sizeof(*in));
  ctx->cached_group = (int32_t)group;
  return 0;
}

// Allocate one block.  Returns 0 when the filesystem is full or an I/O error
// occurs.  Block 0 is never handed out by a valid ext2 bitmap, so it doubles
// as the error value.
uint32_t ext2_alloc_block(ext2_context_t *ctx) {
  uint32_t group;
  uint32_t bitmap_bits;
  uint32_t total_blocks;
  uint32_t first_data;
  uint32_t blocks_per_group;
  uint32_t block_size;

  if (ctx == NULL)
    return 0;

  total_blocks = ctx->superblock->main.total_blocks;
  first_data = ctx->first_data_block;
  blocks_per_group = ctx->blocks_per_group;
  block_size = ctx->block_size;
  bitmap_bits = block_size * 8;

  for (group = 0; group < ctx->group_count; ++group) {
    ext2_bgd_t bgd;
    uint32_t group_start;
    uint32_t limit;
    uint32_t bit;

    if (ext2_read_bgd(ctx, group, &bgd) != 0)
      return 0;
    if (bgd.num_unalloc_blocks == 0)
      continue;

    group_start = first_data + group * blocks_per_group;
    if (group_start >= total_blocks)
      continue;

    limit = total_blocks - group_start;
    if (limit > blocks_per_group)
      limit = blocks_per_group;
    if (limit > bitmap_bits)
      limit = bitmap_bits;

    if (ext2_read_block(ctx, bgd.block_usage_bitmap_addr, ctx->aux_buf) != 0)
      return 0;

    for (bit = 0; bit < limit; ++bit) {
      uint32_t block;
      uint8_t mask;

      mask = (uint8_t)(1u << (bit & 7u));
      if (ctx->aux_buf[bit >> 3] & mask)
        continue;

      block = group_start + bit;
      if (block == 0 || block >= total_blocks)
        continue;

      ctx->aux_buf[bit >> 3] |= mask;
      if (ext2_write_block(ctx, bgd.block_usage_bitmap_addr, ctx->aux_buf) !=
          0)
        return 0;

      if (bgd.num_unalloc_blocks != 0)
        --bgd.num_unalloc_blocks;
      if (ext2_write_bgd(ctx, group, &bgd) != 0)
        return 0;

      if (ctx->superblock->main.num_unallocated_blocks != 0)
        --ctx->superblock->main.num_unallocated_blocks;
      ctx->super_dirty = true;

      // New indirect blocks must start zeroed, and zeroing data blocks keeps
      // old file contents from leaking into holes.
      ext2_bzero(ctx->aux_buf, block_size);
      if (ext2_write_block(ctx, block, ctx->aux_buf) != 0)
        return 0;

      return block;
    }
  }

  return 0;
}

int ext2_flush_super(ext2_context_t *ctx) {
  uint32_t lba;

  if (ctx == NULL || ctx->parent == NULL || ctx->parent->write == NULL)
    return EINVAL;
  if (!ctx->super_dirty)
    return 0;

  lba = ctx->lba_start + 2; // byte 1024 == sector 2 of the partition
  if (ctx->parent->write(ctx->parent, lba, 2, ctx->superblock) != 0)
    return EIO;

  ctx->super_dirty = false;
  return 0;
}
