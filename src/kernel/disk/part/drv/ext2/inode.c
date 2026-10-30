// inode.c
// ----------------------
// Handle navigation of inodes
//
// These are internal functions; not meant to face the rest of krnl

#include "disk/disk.h"
#include "disk/part/drv/ext2/include/def.h"
#include "ext2.h"

int ext2_inode_r_inode(ext2_context_t *ctx, int inode_n, void *buf) {
  uint32_t x = (inode_n - 1) * ((inode_n - 1) / ((ext2_superblock_t*)(ctx->superblock))->block_group_num_inodes);
  uint32_t bs = 1024 << ((ext2_superblock_t*)ctx->superblock)->block_group_num_inodes;
  uint32_t 
  return 0;  
}
