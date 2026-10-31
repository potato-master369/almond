#ifndef EXT2_DEF_H
#define EXT2_DEF_H
#include "../../../../../kernel_types.h"

// On-disk definitions for the second extended filesystem (ext2).
//
// References:
//   - Ext2 - OSDev Wiki
//   - NetBSD sys/ufs/ext2fs
//
// Everything in this header describes little-endian on-disk data.  Almond
// only targets little-endian x86, so the structures can be used directly.
// This is the classic ext2 format only: no journal, no extents, no 64-bit
// block numbers and no flex_bg groups.

#define SECTOR_SIZE 512

// inode block layout
#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK 12
#define EXT2_DIND_BLOCK 13
#define EXT2_TIND_BLOCK 14
#define EXT2_N_BLOCKS 15

// well-known inode numbers
#define EXT2_BADBLK_INO 1
#define EXT2_ROOT_INO 2
#define EXT2_ACL_IDX_INO 3
#define EXT2_ACL_DATA_INO 4
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO 6
#define EXT2_RESIZE_INO 7
#define EXT2_JOURNAL_INO 8
#define EXT2_FIRST_INO 11

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SUPERBLOCK_SIZE 1024

#define EXT2_MIN_BLOCK_SIZE 1024
#define EXT2_MAX_BLOCK_SIZE 4096 // practical real-world ceiling for ext2
#define EXT2_MIN_INODE_SIZE 128
#define EXT2_MAX_SYMLINK_LEN (EXT2_N_BLOCKS * 4)

#define EXT2_MAGIC 0xEF53

// superblock state
#define EXT2_VALID_FS 1
#define EXT2_ERROR_FS 2

// behaviour on errors
#define EXT2_ERRORS_CONTINUE 1
#define EXT2_ERRORS_RO 2
#define EXT2_ERRORS_PANIC 3

// creator operating systems
#define EXT2_OS_LINUX 0
#define EXT2_OS_HURD 1
#define EXT2_OS_MASIX 2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES 4

// compatible feature flags (safe to ignore if not understood)
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC 0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES 0x0002
#define EXT2_FEATURE_COMPAT_HAS_JOURNAL 0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR 0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE 0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX 0x0020

// incompatible feature flags (must be understood to mount)
#define EXT2_FEATURE_INCOMPAT_COMPRESSION 0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE 0x0002
#define EXT2_FEATURE_INCOMPAT_RECOVER 0x0004
#define EXT2_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG 0x0010
#define EXT2_FEATURE_INCOMPAT_EXTENTS 0x0040
#define EXT2_FEATURE_INCOMPAT_64BIT 0x0080
#define EXT2_FEATURE_INCOMPAT_MMP 0x0100
#define EXT2_FEATURE_INCOMPAT_FLEX_BG 0x0200

// read-only-compatible feature flags (we may read, but not safely write)
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE 0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR 0x0004
#define EXT2_FEATURE_RO_COMPAT_HUGE_FILE 0x0008
#define EXT2_FEATURE_RO_COMPAT_GDT_CSUM 0x0010
#define EXT2_FEATURE_RO_COMPAT_DIR_NLINK 0x0020
#define EXT2_FEATURE_RO_COMPAT_EXTRA_ISIZE 0x0040

// inode type and permission bits (identical to POSIX st_mode)
#define EXT2_S_IFMT 0170000
#define EXT2_S_IFSOCK 0140000
#define EXT2_S_IFLNK 0120000
#define EXT2_S_IFREG 0100000
#define EXT2_S_IFBLK 0060000
#define EXT2_S_IFDIR 0040000
#define EXT2_S_IFCHR 0020000
#define EXT2_S_IFIFO 0010000
#define EXT2_S_ISUID 0004000
#define EXT2_S_ISGID 0002000
#define EXT2_S_ISVTX 0001000
#define EXT2_S_IRUSR 0000400
#define EXT2_S_IWUSR 0000200
#define EXT2_S_IXUSR 0000100
#define EXT2_S_IRWXU 0000700
#define EXT2_S_IRWXG 0000070
#define EXT2_S_IRWXO 0000007

// inode flags (classic ext2 only)
#define EXT2_INODE_FLAG_SECRM 0x00000001
#define EXT2_INODE_FLAG_UNRM 0x00000002
#define EXT2_INODE_FLAG_COMPR 0x00000004
#define EXT2_INODE_FLAG_SYNC 0x00000008
#define EXT2_INODE_FLAG_IMMUTABLE 0x00000010
#define EXT2_INODE_FLAG_APPEND 0x00000020
#define EXT2_INODE_FLAG_NODUMP 0x00000040
#define EXT2_INODE_FLAG_NOATIME 0x00000080
#define EXT2_INODE_FLAG_INDEX 0x00001000
#define EXT2_INODE_FLAG_IMAGIC 0x00002000
#define EXT2_INODE_FLAG_JOURNAL_DATA 0x00004000
#define EXT2_INODE_FLAG_NOTAIL 0x00008000
#define EXT2_INODE_FLAG_DIRSYNC 0x00010000
#define EXT2_INODE_FLAG_TOPDIR 0x00020000

// directory entry file types
#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO 5
#define EXT2_FT_SOCK 6
#define EXT2_FT_SYMLINK 7

// directory entry helpers
#define EXT2_DIR_PAD 4
#define EXT2_DIR_ROUND (EXT2_DIR_PAD - 1)
#define EXT2_DIR_REC_LEN(name_len)                                             \
  (((name_len) + 8 + EXT2_DIR_ROUND) & ~EXT2_DIR_ROUND)

// superblock.  Names follow the original driver; note that the field at
// offset 32 is the number of blocks per group, and the field at offset 40 is
// the number of inodes per group.
typedef struct __attribute__((packed)) {
  uint32_t num_inodes;
  uint32_t total_blocks;
  uint32_t num_su_reserved;
  uint32_t num_unallocated_blocks;
  uint32_t num_unallocated_inodes;
  uint32_t superblock_block_no; // first data block
  uint32_t log2_block_size;
  uint32_t log2_fragment_size;
  uint32_t block_group_num_groups; // blocks per group
  uint32_t block_group_num_fragments;
  uint32_t block_group_num_inodes; // inodes per group
  uint32_t last_mount_time;
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
  char reserved[788]; // pad to exactly 1024 bytes
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

// The inode is naturally laid out without padding, so it is not marked packed.
// That keeps its block-pointer fields properly aligned for the mapper.
typedef struct {
  uint16_t permissions;
  uint16_t uid;
  uint32_t size_lower;
  uint32_t access_time;
  uint32_t create_time;
  uint32_t modify_time;
  uint32_t delete_time;
  uint16_t gid;
  uint16_t num_hard_links;
  uint32_t num_sectors; // 512-byte sectors, not ext2 blocks
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
  uint32_t gen;        // NFS generation
  uint32_t acl;        // rev < 1 is reserved
  uint32_t size_upper; // rev < 1 is reserved; high 32 bits of size
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
  uint8_t name_length; // LS 8 bits; also low byte of rev0 16-bit namelen
  uint8_t type_indicator; // rev0: high byte of namelen (always zero)
  char name[];
} ext2_dirent_t;

// Compile-time sanity checks: these structures are read straight off disk.
typedef char ext2_superblock_size_check
    [(sizeof(ext2_superblock_full_t) == EXT2_SUPERBLOCK_SIZE) ? 1 : -1];
typedef char ext2_inode_size_check
    [(sizeof(ext2_inode_t) == EXT2_MIN_INODE_SIZE) ? 1 : -1];
typedef char ext2_bgd_size_check[(sizeof(ext2_bgd_t) == 32) ? 1 : -1];

#endif
