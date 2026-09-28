// ext2.c
// -------------------
// MIT License; Copyright (C) potato-master369-
// ext2 driver for ABL
#include "ext2.h"
#include "../bl_types.h"
#include "../bl_vga.h"
#include "../string.h"
#include "disk.h"

// disk info struct
extern disk_t disk_info[DISK_MAX];
uint8_t read_buf[512];

#define SECTOR_SIZE 512
#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK 12
#define EXT2_DIND_BLOCK 13
#define EXT2_TIND_BLOCK 14
#define EXT2_ROOT_INO 2
#define EXT2_MAX_BLOCK_SIZE                                                    \
  4096 // practical real-world ceiling for ext2 on 32-bit

// set by ext2_read_superblock(), used by every block-math helper below
uint32_t ext2_block_size;
uint32_t ext2_gdt_block;
uint32_t ext2_sectors_per_block;

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

uint8_t ext2_active_disk_n = 255;
uint8_t ext2_active_part_n = 255;
ext2_superblock_full_t ext2_active_superblock;

// helper functions
uint8_t ext2_read_superblock(uint8_t disk_n, uint8_t part_n) {
  uint8_t res = 0;
  res = disk_read_w_part(disk_n, part_n, 2, (void *)&ext2_active_superblock);
  if (res != 0)
    return res;
  res = disk_read_w_part(disk_n, part_n, 3,
                         (void *)((char *)&ext2_active_superblock + 512));
  if (res != 0)
    return res;

  // block size = 1024 << s_log_block_size, per spec
  ext2_block_size = 1024u << ext2_active_superblock.main.log2_block_size;

  if (ext2_block_size > EXT2_MAX_BLOCK_SIZE || ext2_block_size < 1024) {
    return 0x10; // unsupported block size - refuse rather than overrun buffers
  }

  ext2_sectors_per_block = ext2_block_size / SECTOR_SIZE;

  ext2_gdt_block = (ext2_block_size == 1024) ? 2 : 1;

  return 0;
}

// block_num is in ext2's own block numbering (block_size units), NOT sectors.
uint8_t ext2_read_block(uint8_t disk_n, uint8_t part_n, uint32_t block_num,
                        void *buf) {
  uint32_t start_sector = block_num * ext2_sectors_per_block;
  for (uint32_t i = 0; i < ext2_sectors_per_block; i++) {
    uint8_t res = disk_read_w_part(disk_n, part_n, start_sector + i,
                                   (uint8_t *)buf + (i * SECTOR_SIZE));
    if (res != 0)
      return 0x10;
  }
  return 0;
}

uint8_t ext2_write_block(uint8_t disk_n, uint8_t part_n, uint32_t block_num,
                         void *buf) {
  uint32_t start_sector = block_num * ext2_sectors_per_block;
  for (uint32_t i = 0; i < ext2_sectors_per_block; i++) {
    uint8_t res = disk_write_w_part(disk_n, part_n, start_sector + i,
                                    (uint8_t *)buf + (i * SECTOR_SIZE));
    if (res != 0)
      return 0x10;
  }
  return 0;
}

uint8_t ext2_read_bgd(uint8_t disk_n, uint8_t part_n, uint32_t group,
                      ext2_bgd_t *out) {
  uint32_t entries_per_block = ext2_block_size / sizeof(ext2_bgd_t);
  uint32_t block = ext2_gdt_block + (group / entries_per_block);
  uint32_t idx = group % entries_per_block;

  uint8_t blockbuf[EXT2_MAX_BLOCK_SIZE];
  if (ext2_read_block(disk_n, part_n, block, blockbuf) != 0)
    return 0x10;

  ext2_bgd_t *entries = (ext2_bgd_t *)blockbuf;
  *out = entries[idx];
  return 0;
}

uint8_t ext2_get_inode(uint8_t disk_n, uint8_t part_n, uint32_t inode_num,
                       ext2_inode_t *out) {
  if (inode_num == 0)
    return 0x10;

  uint32_t inodes_per_group =
      ext2_active_superblock.main.block_group_num_inodes;
  if (inodes_per_group == 0)
    return 0x10;

  uint32_t group = (inode_num - 1) / inodes_per_group;
  uint32_t index_in_group = (inode_num - 1) % inodes_per_group;

  ext2_bgd_t bgd;
  if (ext2_read_bgd(disk_n, part_n, group, &bgd) != 0)
    return 0x10;

  uint16_t inode_size = (ext2_active_superblock.main.version_major >= 1)
                            ? ext2_active_superblock.extend.sizeof_inode
                            : 128;
  if (inode_size == 0)
    inode_size = 128;

  uint32_t inodes_per_block = ext2_block_size / inode_size;
  if (inodes_per_block == 0)
    return 0x10;

  uint32_t block_offset = index_in_group / inodes_per_block;
  uint32_t offset_in_block = (index_in_group % inodes_per_block) * inode_size;

  uint8_t blockbuf[EXT2_MAX_BLOCK_SIZE];
  if (ext2_read_block(disk_n, part_n,
                      bgd.inode_table_starting_addr + block_offset,
                      blockbuf) != 0)
    return 0x10;

  uint32_t copy_size =
      inode_size < sizeof(ext2_inode_t) ? inode_size : sizeof(ext2_inode_t);
  memcpy(out, blockbuf + offset_in_block, copy_size);
  return 0;
}

// resolve inode to block
// err is set to 1 on failure (unallocated pointer, or read error walking
// indirect blocks).
uint32_t ext2_get_block_num(uint8_t disk_n, uint8_t part_n, ext2_inode_t *inode,
                            uint32_t logical_block, uint8_t *err) {
  *err = 0;
  uint32_t ptrs_per_block = ext2_block_size / 4;

  if (logical_block < EXT2_NDIR_BLOCKS) {
    uint32_t *dbp = &inode->dbp0; // contiguous uint32_t members - safe to index
                                  // like an array
    return dbp[logical_block];
  }
  logical_block -= EXT2_NDIR_BLOCKS;

  if (logical_block < ptrs_per_block) {
    if (inode->sibp == 0) {
      return 0;
    }
    uint32_t indirect[EXT2_MAX_BLOCK_SIZE / 4];
    if (ext2_read_block(disk_n, part_n, inode->sibp, indirect) != 0) {
      *err = 1;
      return 0;
    }
    return indirect[logical_block];
  }
  logical_block -= ptrs_per_block;

  if (logical_block < ptrs_per_block * ptrs_per_block) {
    if (inode->dibp == 0) {
      return 0;
    }
    uint32_t indirect1[EXT2_MAX_BLOCK_SIZE / 4];
    if (ext2_read_block(disk_n, part_n, inode->dibp, indirect1) != 0) {
      *err = 1;
      return 0;
    }

    uint32_t idx1 = logical_block / ptrs_per_block;
    uint32_t idx2 = logical_block % ptrs_per_block;
    if (indirect1[idx1] == 0) {
      return 0;
    }

    uint32_t indirect2[EXT2_MAX_BLOCK_SIZE / 4];
    if (ext2_read_block(disk_n, part_n, indirect1[idx1], indirect2) != 0) {
      *err = 1;
      return 0;
    }
    return indirect2[idx2];
  }

  // dgaf about 3IP cos genuinely when the FUCK do you have a file >= 4GiB
  *err = 1;
  return 0;
}

static inline uint32_t ext2_get_file_size(const ext2_inode_t *inode) {
  uint32_t size = inode->size_lower;
  return size;
}

// looks up dirent
uint32_t ext2_lookup_in_dir(uint8_t disk_n, uint8_t part_n,
                            ext2_inode_t *dir_inode, const char *name,
                            uint8_t name_len) {
  uint32_t *dbp = &dir_inode->dbp0;
  uint8_t blockbuf[EXT2_MAX_BLOCK_SIZE];

  for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
    if (dbp[i] == 0)
      continue;
    if (ext2_read_block(disk_n, part_n, dbp[i], blockbuf) != 0)
      return 0;

    uint32_t off = 0;
    while (off < ext2_block_size) {
      ext2_dirent_t *de = (ext2_dirent_t *)(blockbuf + off);
      if (de->size == 0)
        break; // corrupt / end of usable entries in this block

      if (de->inode != 0 && de->name_length == name_len) {
        uint8_t match = 1;
        for (int c = 0; c < name_len; c++) {
          if (de->name[c] != name[c]) {
            match = 0;
            break;
          }
        }
        if (match)
          return de->inode;
      }
      off += de->size;
    }
  }
  return 0;
}

// thingy to read paths
uint32_t ext2_resolve_path(uint8_t disk_n, uint8_t part_n, const char *path,
                           ext2_inode_t *out_inode) {
  ext2_inode_t current_inode;
  if (ext2_get_inode(disk_n, part_n, EXT2_ROOT_INO, &current_inode) != 0)
    return 0;

  const char *p = path;
  if (*p == '/')
    p++;

  char component[256];
  while (*p) {
    int len = 0;
    while (p[len] != '/' && p[len] != '\0' && len < 255)
      len++;

    if (len == 0) {
      p++;
      continue;
    } // lets us be lazy and do a//b

    for (int i = 0; i < len; i++)
      component[i] = p[i];
    component[len] = '\0';

    uint32_t next_inode_num = ext2_lookup_in_dir(disk_n, part_n, &current_inode,
                                                 component, (uint8_t)len);
    if (next_inode_num == 0)
      return 0;

    if (ext2_get_inode(disk_n, part_n, next_inode_num, &current_inode) != 0)
      return 0;

    p += len;
    if (*p == '/')
      p++;

    // we reached the end
    if (*p == '\0') {
      *out_inode = current_inode;
      return next_inode_num;
    }
  }

  // path was just "/" - return root
  *out_inode = current_inode;
  return EXT2_ROOT_INO;
}

uint8_t ext2_read_inode_data(uint8_t disk_n, uint8_t part_n,
                             ext2_inode_t *inode, void *buf,
                             uint32_t read_size) {
  int32_t file_size = ext2_get_file_size(inode);

  // Clamp read_size to remaining file size
  if (read_size > file_size) {
    read_size = (uint32_t)file_size;
  }
  uint32_t bytes_done = 0;
  uint32_t logical_block = 0;
  uint8_t blockbuf[EXT2_MAX_BLOCK_SIZE];

  while (bytes_done < read_size) {
    uint8_t err = 0;
    uint32_t block_num =
        ext2_get_block_num(disk_n, part_n, inode, logical_block, &err);
    if (err)
      return 0x10;

    uint32_t chunk = read_size - bytes_done;
    if (chunk > ext2_block_size)
      chunk = ext2_block_size;

    // Sparse file holes read as zeroes instead of disk blocks.
    if (block_num == 0) {
      for (uint32_t i = 0; i < chunk; i++)
        ((uint8_t *)buf)[bytes_done + i] = 0;
      bytes_done += chunk;
      logical_block++;
      continue;
    }

    if (ext2_read_block(disk_n, part_n, block_num, blockbuf) != 0)
      return 0x10;

    memcpy((uint8_t *)buf + bytes_done, blockbuf, chunk);

    bytes_done += chunk;
    logical_block++;
  }
  return 0;
}

uint8_t ext2_write_inode_data(uint8_t disk_n, uint8_t part_n,
                              ext2_inode_t *inode, void *buf,
                              uint32_t write_size) {
  uint32_t bytes_done = 0;
  uint32_t logical_block = 0;
  uint8_t blockbuf[EXT2_MAX_BLOCK_SIZE];

  while (bytes_done < write_size) {
    uint8_t err = 0;
    uint32_t block_num =
        ext2_get_block_num(disk_n, part_n, inode, logical_block, &err);
    if (err || block_num == 0)
      return 0x10;

    uint32_t chunk = write_size - bytes_done;
    if (chunk > ext2_block_size)
      chunk = ext2_block_size;

    if (chunk < ext2_block_size) {
      // partial trailing block - preserve the rest of its existing contents
      if (ext2_read_block(disk_n, part_n, block_num, blockbuf) != 0)
        return 0x10;
    }

    memcpy(blockbuf, (uint8_t *)buf + bytes_done, chunk);

    if (ext2_write_block(disk_n, part_n, block_num, blockbuf) != 0)
      return 0x10;

    bytes_done += chunk;
    logical_block++;
  }
  return 0;
}

// return values for the next 2 things:
// 0x10 - error
uint8_t ext2_part_read(uint8_t disk_n, uint8_t part_n, const char *path,
                       void *buf, uint32_t read_size) {
  if (ext2_active_disk_n != disk_n || ext2_active_part_n != part_n) {
    if (ext2_read_superblock(disk_n, part_n) != 0) {
      bl_vga_write(" ext2: 0x10\n", 0x04);
      return 0x10;
    }
    ext2_active_disk_n = disk_n;
    ext2_active_part_n = part_n;
  }

  ext2_inode_t inode;
  if (ext2_resolve_path(disk_n, part_n, path, &inode) == 0)
    return 0x10;
  return ext2_read_inode_data(disk_n, part_n, &inode, buf, read_size);
}

uint8_t ext2_part_write(uint8_t disk_n, uint8_t part_n, const char *path,
                        void *buf, uint32_t read_size) {
  if (ext2_active_disk_n != disk_n || ext2_active_part_n != part_n) {
    if (ext2_read_superblock(disk_n, part_n) != 0)
      return 0x10;
    ext2_active_disk_n = disk_n;
    ext2_active_part_n = part_n;
  }

  ext2_inode_t inode;
  if (ext2_resolve_path(disk_n, part_n, path, &inode) == 0)
    return 0x10;
  return ext2_write_inode_data(disk_n, part_n, &inode, buf, read_size);
}

int32_t ext2_part_get_filesize(uint8_t disk_n, uint8_t part_n, const char *path) {
  if (ext2_active_disk_n != disk_n || ext2_active_part_n != part_n) {
    if (ext2_read_superblock(disk_n, part_n) != 0)
      return -1;
    ext2_active_disk_n = disk_n;
    ext2_active_part_n = part_n;
  }

  ext2_inode_t inode;
  if (ext2_resolve_path(disk_n, part_n, path, &inode) == 0)
	  return -1;
  return ext2_get_file_size(&inode);
}

// returns 0 if IS ext2, and 1 if not.
uint8_t ext2_verify(uint8_t disk_num, uint8_t part_num) {
  if (disk_info[disk_num].drv_id != DRV_ID_NULL &&
      disk_info[disk_num].mbr[part_num][4] != 0) {
    disk_read_w_part(disk_num, part_num, 2, read_buf);
    if (*(uint16_t *)(&read_buf[56]) == 0xEF53) {
      // valid ext2 fs
      bl_vga_write(" ext2: found\n", 0x07);
      disk_info[disk_num].part_read[part_num] = ext2_part_read;
      disk_info[disk_num].part_write[part_num] = ext2_part_write;
      disk_info[disk_num].part_get_filesize[part_num] = ext2_part_get_filesize;
    } else {
      return 1;
    }
    return 0;
  } else {
    return 1;
  }
}
