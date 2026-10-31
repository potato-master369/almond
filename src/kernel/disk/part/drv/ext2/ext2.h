#ifndef EXT2_H
#define EXT2_H
#define FSID_DRV_EXT2 0x65787432u // uint32_t size; "ext2"

#include "disk/disk.h"
#include "include/def.h"

// A filesystem ID is just the disk and partition number packed together, as
// suggested by almond_filehandle_t.  The upper 16 bits are the disk, the
// lower 16 bits are the partition.
#define EXT2_FSID(disk_n, part_n)                                              \
  ((((uint32_t)(disk_n) & 0xFFFFu) << 16) | ((uint32_t)(part_n) & 0xFFFFu))

#define EXT2_MAX_SYMLINK_DEPTH 8

// Per-mounted-filesystem context.  This lives behind partition_info_t.resv.
// All large buffers are allocated on the heap; only the small linked-list head
// in ext2.c lives in BSS.
typedef struct ext2_context {
  ext2_superblock_full_t *superblock; // 1KiB, freed when unmounting
  ext2_bgd_t *block_group_desc;       // cached group descriptor
  disk_info_t *parent;
  struct ext2_context *next;

  uint8_t *block_buf; // one filesystem block, used for metadata/data
  uint8_t *aux_buf;   // one filesystem block, used by the block allocator

  uint32_t fsid;

  uint32_t lba_start;       // first sector of the partition
  uint32_t part_n;
  uint32_t block_size;
  uint32_t sectors_per_block;
  uint32_t inode_size;
  uint32_t inodes_per_group;
  uint32_t blocks_per_group;
  uint32_t group_count;
  uint32_t gdt_block; // first block of the primary group descriptor table
  uint32_t gdt_entries_per_block;
  uint32_t ptrs_per_block;
  uint32_t first_data_block;
  uint32_t inodes_per_block;
  uint32_t inode_blocks_per_group;
  int32_t cached_group;
  bool_t super_dirty;
} ext2_context_t;

// Per-open-file context.  This lives behind almond_filehandle_t.resv.  The
// cursor is deliberately kept here (and mirrored into fh->pos) so a read or
// write just consumes the saved cursor instead of taking an explicit offset.
typedef struct {
  ext2_context_t *fs;
  ext2_inode_t inode; // cached copy of the on-disk inode
  uint32_t inode_n;
  uint64_t cursor;
  uint32_t flags;
} ext2_file_t;

// ext2.c
int ext2_mount_part(partition_info_t *target, disk_info_t *parent,
                    uint8_t part_n);
int ext2_umount_part(partition_info_t *target, uint8_t part_n);
ext2_context_t *ext2_context_from_fsid(uint32_t fsid);
uint64_t ext2_now(void);

// partition-level callbacks installed by ext2_mount_part()
int ext2_read(almond_filehandle_t *fh, void *to, int n);
int ext2_write(almond_filehandle_t *fh, void *from, int n);
int ext2_openstream(almond_filehandle_t *fh, const char *filename);
int ext2_closestream(almond_filehandle_t *fh);
int ext2_stat(almond_stat_t *out, const char *path);
int ext2_dir_index(almond_filehandle_t *dir, uint32_t index,
                   almond_dirent_t *out);
int ext2_dir_count(almond_filehandle_t *dir, uint32_t *out);

// block.c
int ext2_read_block(ext2_context_t *ctx, uint32_t block, void *buf);
int ext2_write_block(ext2_context_t *ctx, uint32_t block, const void *buf);
int ext2_read_bgd(ext2_context_t *ctx, uint32_t group, ext2_bgd_t *out);
int ext2_write_bgd(ext2_context_t *ctx, uint32_t group,
                   const ext2_bgd_t *in);
uint32_t ext2_alloc_block(ext2_context_t *ctx);
int ext2_flush_super(ext2_context_t *ctx);

// inode.c
int ext2_read_inode(ext2_context_t *ctx, uint32_t inode_n, ext2_inode_t *out);
int ext2_write_inode(ext2_context_t *ctx, uint32_t inode_n,
                     const ext2_inode_t *in);
uint64_t ext2_inode_size(const ext2_inode_t *inode);
uint16_t ext2_inode_type(const ext2_inode_t *inode);
int ext2_resolve_path(ext2_context_t *ctx, const char *path, uint32_t *out_ino,
                      ext2_inode_t *out_inode);

// dir.c
int ext2_dir_lookup(ext2_context_t *ctx, const ext2_inode_t *dir,
                    const char *name, uint8_t name_len, uint32_t *out_ino);
int ext2_dir_count_inode(ext2_context_t *ctx, const ext2_inode_t *dir,
                         uint32_t *out);
int ext2_dir_index_inode(ext2_context_t *ctx, const ext2_inode_t *dir,
                         uint32_t index, almond_dirent_t *out);

// file.c
int ext2_bmap(ext2_context_t *ctx, ext2_inode_t *inode, uint32_t logical,
              bool_t create, uint32_t *phys, bool_t *inode_dirty);
int ext2_read_file(ext2_context_t *ctx, ext2_inode_t *inode, uint64_t offset,
                   void *buf, uint32_t count);
int ext2_write_file(ext2_context_t *ctx, ext2_inode_t *inode, uint64_t offset,
                    const void *buf, uint32_t count, bool_t *inode_dirty);

#endif
