#ifndef EXT2_DEF_H
#define EXT2_DEF_H
#include "../../../../../kernel_types.h"

// so basically these are the structs that we need for
// ext2. Put that stuff here.

#define SECTOR_SIZE 512
#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK 12
#define EXT2_DIND_BLOCK 13
#define EXT2_TIND_BLOCK 14
#define EXT2_ROOT_INO 2
#define EXT2_MAX_BLOCK_SIZE                                                    \
  4096 // practical real-world ceiling for ext2 on 32-bit

// struct for blocks/inodes
typedef struct __attribute__((packed)) {
  uint32_t num_inodes;
  uint32_t total_blocks;
  uint32_t num_su_reserved;
  uint32_t num_unallocated_blocks;
  uint32_t num_unallocated_inodes;
  uint32_t superblock_block_no;
  uint32_t log2_block_size;
  uint32_t log2_fragment_size;
  uint32_t block_group_num_groups;
  uint32_t block_group_num_fragments;
  uint32_t block_group_num_inodes;
  uint32_t last_mount_time; // ehhhhhh who cares 笑っ
  uint32_t last_write_time;
  uint16_t num_post_fsck_mounts;
  uint16_t num_allowed_fsck_mounts;
  uint16_t sig;
  uint16_t state; // 1 = clean; 2 = error
  uint16_t onerr;
  uint16_t version_minor;
  uint32_t fsck_time;
  uint32_t forced_fsck_interval;
  uint32_t os_id; // 0 = linux
  uint32_t version_major;
  uint16_t reserved_uid;
  uint16_t reserved_gid;
} ext2_superblock_t;

// these fields are only present if version_major >= 1
typedef struct __attribute__((packed)) {
  uint32_t first_non_reserved_inode;
  uint16_t sizeof_inode; // in bytes
  uint16_t backup_block_group;
  uint32_t optional_features;
  uint32_t required_features;
  uint32_t ro_compat_features;
  uint8_t blkid[16];
  char volume_name[16]; // null-terminated
  char last_mount_path[64];
  uint32_t compress_algo;
  uint8_t
      num_blocks_files; // the following 2 are numbers of blocks to preallocate
  uint8_t num_blocks_dirs;
  uint16_t unused_1;
  uint8_t journal_id[16];
  uint32_t journal_inode;
  uint32_t journal_device;
  uint32_t head_orphan_inode_list;
} ext2_extended_superblock_t;

typedef struct __attribute__((packed)) {
  ext2_superblock_t main;
  ext2_extended_superblock_t extend;
  char reserved[784]; // pad to 1024 bytes.
} ext2_superblock_full_t;

typedef struct __attribute__((packed)) {
  uint32_t block_usage_bitmap; // block address
  uint32_t inode_usage_bitmap;
  uint32_t inode_table;
  uint16_t num_unallocated_blocks;
  uint16_t num_unallocated_inodes;
  uint16_t num_directories;
  uint8_t reserved[14];
} ext2_block_descriptor_t;

typedef struct __attribute__((packed)) {
  uint16_t permissions;
  uint16_t uid;
  uint32_t size_lower;
  uint32_t access_time;
  uint32_t create_time;
  uint32_t modify_time;
  uint32_t delete_time;
  uint16_t gid;
  uint16_t num_hard_links;
  uint32_t num_sectors; // LBA sectors
  uint32_t flags;
  uint32_t ossv1; // dont touch
  uint32_t dbp0;
  uint32_t dbp1;
  uint32_t dbp2;
  uint32_t dbp3;
  uint32_t dbp4;
  uint32_t dbp5;
  uint32_t dbp6;
  uint32_t dbp7;
  uint32_t dbp8;
  uint32_t dbp9;
  uint32_t dbp10;
  uint32_t dbp11;
  uint32_t sibp;
  uint32_t dibp;
  uint32_t tibp;
  uint32_t gen;        // NFS
  uint32_t acl;        // rev < 1 is reserved
  uint32_t size_upper; // rev < 1 is reserved
  uint32_t fragment_address;
  uint8_t ossv[12];
} ext2_inode_t;

typedef struct __attribute__((packed)) {
  uint32_t block_usage_bitmap_addr;
  uint32_t inode_usage_bitmap_addr;
  uint32_t inode_table_starting_addr;
  uint16_t num_unalloc_blocks;
  uint16_t num_unalloc_inodes;
  uint16_t num_dirs;
  char reserved[14];
} ext2_bgd_t;

typedef struct __attribute__((packed)) {
  uint32_t inode;
  uint16_t size;
  uint8_t name_length; // LS 8 bits
  uint8_t type_indicator;
  char name[];
} ext2_dirent_t;
#endif
