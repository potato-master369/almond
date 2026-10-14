#ifndef EXT2_H
#define EXT2_H
#define FSID_DRV_EXT2 0x65787432u // uint32_t size; "ext2"

// fs context
typedef struct {
  void *superblock; // must  be freed when unmounting
  void *block_group_desc; // must be freed when unmounting
} ext2_context_t;
#endif
