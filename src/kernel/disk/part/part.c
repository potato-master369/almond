// part.c
// -----------------------
// Partition/filesystem dispatch layer.
//
// File handles are owned by the kernel; this file only allocates their IDs and
// forwards operations to the filesystem driver installed in the matching
// partition_info_t.
#include "part.h"
#include "disk/disk.h"
#include "drv/ext2/ext2.h"
#include "helpers/string.h"
#include "messaging/messaging.h"

#define PART_WORDS ((PART_MAX_OPEN + 31u) / 32u)
#define PART_TAIL_BITS (PART_MAX_OPEN % 32u)
#define PART_DRIVE_COUNT 26

// part_mount_drive() results
#define PART_MOUNT_UNAVAILABLE (-1)
#define PART_MOUNT_FAILED (-2)

extern partition_info_t part_db[PART_DRIVE_COUNT];
extern disk_info_t disk_db[DISK_MAX];

static uint32_t part_bitmap[PART_WORDS];
static uint32_t part_next;

static inline uint32_t part_valid_mask(uint32_t w) {
  if (PART_TAIL_BITS && w == PART_WORDS - 1)
    return (1u << PART_TAIL_BITS) - 1u;
  return ~0u;
}

static uint32_t part_gen_handle_id(void) {
  uint32_t start_w = part_next >> 5;
  uint32_t start_b = part_next & 31u;

  for (uint32_t k = 0; k <= PART_WORDS; k++) {
    uint32_t w = start_w + k;
    if (w >= PART_WORDS)
      w -= PART_WORDS;

    uint32_t freebits = ~part_bitmap[w] & part_valid_mask(w);

    if (k == 0)
      freebits &= (~0u << start_b);
    else if (k == PART_WORDS)
      freebits &= (1u << start_b) - 1u;

    if (freebits) {
      uint32_t b = (uint32_t)__builtin_ctz(freebits);
      uint32_t idx = (w << 5) + b;

      part_bitmap[w] |= (1u << b);
      part_next = (idx + 1 >= PART_MAX_OPEN) ? 0 : idx + 1;
      return idx + 1;
    }
  }
  return 0;
}

static void part_dismember_handle_id(uint32_t id) {
  if (id == 0 || id > PART_MAX_OPEN)
    return;
  id--;
  part_bitmap[id >> 5] &= ~(1u << (id & 31u));
}

static uint32_t part_disk_index(disk_info_t *disk) {
  uint32_t i;

  if (disk == NULL)
    return 0;
  for (i = 0; i < DISK_MAX; ++i) {
    if (&disk_db[i] == disk)
      return i;
  }
  return 0;
}

static uint32_t part_fsid_for(partition_info_t *part) {
  if (part == NULL || part->parent == NULL)
    return 0;
  return ((part_disk_index(part->parent) & 0xFFFFu) << 16) |
         ((uint32_t)part->part_n & 0xFFFFu);
}

static partition_info_t *part_from_fsid(uint32_t fsid) {
  uint32_t disk_n = (fsid >> 16) & 0xFFFFu;
  uint32_t part_n = fsid & 0xFFFFu;
  uint32_t i;

  if (disk_n >= DISK_MAX || part_n > 0xFFu)
    return NULL;

  for (i = 0; i < PART_DRIVE_COUNT; ++i) {
    partition_info_t *part = &part_db[i];

    if (!part->present)
      continue;
    if (part->parent == &disk_db[disk_n] && (uint32_t)part->part_n == part_n)
      return part;
  }
  return NULL;
}

// Paths may select a drive with the classic "A:/path" form.  If there is no
// drive prefix, absolute paths are tried against every mounted filesystem.
static int part_drive_from_path(const char *path, const char **path_out) {
  char c;

  if (path == NULL || path_out == NULL)
    return -1;

  if (path[0] != '\0' && path[1] == ':') {
    c = path[0];

    if (c >= 'a' && c <= 'z')
      c = (char)(c - 'a' + 'A');

    if (c >= 'A' && c <= 'Z') {
      *path_out = path + 2;
      return c - 'A';
    }
  }

  *path_out = path;
  return -1;
}

static bool_t part_mode_valid(const char *mode) {
  const char *p;
  bool_t has_access = false;

  if (mode == NULL || mode[0] == '\0')
    return false;

  for (p = mode; *p != '\0'; ++p) {
    if (*p == 'r' || *p == 'w' || *p == 'a')
      has_access = true;
    else if (*p != '+' && *p != 'b' && *p != 't')
      return false;
  }
  return has_access;
}

static bool_t part_mode_append(const char *mode) {
  while (mode != NULL && *mode != '\0') {
    if (*mode++ == 'a')
      return true;
  }
  return false;
}

static int part_open_on(partition_info_t *part, almond_filehandle_t *out,
                        const char *path, const char *mode) {
  int rc;

  if (part == NULL || !part->present || part->openstream == NULL) {
    message_send_message("ERR:part: open on unavailable partition\n");
    return ENOENT;
  }

  out->fsid = part_fsid_for(part);
  out->resv = 0;
  out->inode_n = 0;
  out->pos = 0;
  out->generation_count = 0;

  rc = part->openstream(out, path);
  if (rc != 0) {
    out->resv = 0;
    out->inode_n = 0;
    out->pos = 0;
    return rc;
  }

  // Append mode starts at the end of the file.  The ext2 driver mirrors
  // fh->pos into its private cursor, so updating pos is enough.
  if (part_mode_append(mode) && part->stat != NULL) {
    almond_stat_t st;

    st.fsid = out->fsid;
    if (part->stat(&st, path) == 0)
      out->pos = st.size;
  }

  return 0;
}

// returns if failed
int kopen(almond_filehandle_t *out, const char *path, const char *mode) {
  const char *fs_path;
  int drive;
  int rc;
  uint32_t i;

  if (out == NULL || path == NULL || !part_mode_valid(mode)) {
    message_send_message("ERR:part: invalid open request\n");
    return EINVAL;
  }

  out->id = 0;
  out->fsid = 0;
  out->resv = 0;
  out->generation_count = 0;
  out->pos = 0;
  out->inode_n = 0;

  out->id = part_gen_handle_id();
  if (out->id == 0) {
    message_send_message("ERR:part: file handle table full\n");
    return EVFLOW;
  }

  drive = part_drive_from_path(path, &fs_path);
  if (drive >= 0) {
    if (drive >= PART_DRIVE_COUNT)
      rc = ENOENT;
    else
      rc = part_open_on(&part_db[drive], out, fs_path, mode);
  } else {
    rc = ENOENT;
    for (i = 0; i < PART_DRIVE_COUNT; ++i) {
      if (!part_db[i].present || part_db[i].openstream == NULL)
        continue;
      rc = part_open_on(&part_db[i], out, path, mode);
      if (rc == 0)
        break;
    }
  }

  if (rc != 0) {
    message_send_message("ERR:part: open failed\n");
    part_dismember_handle_id(out->id);
    out->id = 0;
    out->fsid = 0;
    out->resv = 0;
    out->inode_n = 0;
    out->pos = 0;
  }
  return rc;
}

int kclose(almond_filehandle_t *f) {
  partition_info_t *part;
  int rc = 0;

  if (f == NULL || f->id == 0 || f->id > PART_MAX_OPEN) {
    message_send_message("ERR:part: invalid close request\n");
    return EINVAL;
  }

  part = part_from_fsid(f->fsid);
  if (part != NULL && part->closestream != NULL) {
    rc = part->closestream(f);
    if (rc != 0)
      message_send_message("ERR:part: close failed\n");
  }

  part_dismember_handle_id(f->id);
  f->id = 0;
  f->fsid = 0;
  f->resv = 0;
  f->generation_count = 0;
  f->pos = 0;
  f->inode_n = 0;
  return rc;
}

int kread(almond_filehandle_t *fh, void *buf, uint32_t count) {
  partition_info_t *part;

  if (fh == NULL || fh->id == 0 || (buf == NULL && count != 0)) {
    message_send_message("ERR:part: invalid read request");
    return EINVAL;
  }
  if (count == 0)
    return 0;
  if (count > 0x7FFFFFFFu)
    return EVFLOW;

  part = part_from_fsid(fh->fsid);
  if (part == NULL || part->read == NULL) {
    message_send_message("ERR:part: no such partition\n");
    return ENOENT;
  }
  return part->read(fh, buf, (int)count);
}

int kwrite(almond_filehandle_t *fh, const void *buf, uint32_t count) {
  partition_info_t *part;

  if (fh == NULL || fh->id == 0 || (buf == NULL && count != 0)) {
    message_send_message("ERR:part: invalid write request\n");
    return EINVAL;
  }
  if (count == 0)
    return 0;
  if (count > 0x7FFFFFFFu)
    return EVFLOW;

  part = part_from_fsid(fh->fsid);
  if (part == NULL || part->write == NULL) {
    message_send_message("ERR:part: no such partition\n");
    return ENOENT;
  }
  return part->write(fh, (void *)buf, (int)count);
}

int kdir_index(almond_filehandle_t *dir, uint32_t index, almond_dirent_t *out) {
  partition_info_t *part;

  if (dir == NULL || dir->id == 0 || out == NULL)
    return EINVAL;

  part = part_from_fsid(dir->fsid);
  if (part == NULL || part->dir_index == NULL)
    return ENOENT;
  return part->dir_index(dir, index, out);
}

int kdir_count(almond_filehandle_t *dir, uint32_t *out) {
  partition_info_t *part;

  if (dir == NULL || dir->id == 0 || out == NULL)
    return EINVAL;

  part = part_from_fsid(dir->fsid);
  if (part == NULL || part->dir_count == NULL)
    return ENOENT;
  return part->dir_count(dir, out);
}

// Older callers pass the directory entry buffer as a byte pointer.
int kdirent_read(almond_filehandle_t *f, const int n, const uint8_t *to) {
  if (n < 0)
    return EINVAL;
  return kdir_index(f, (uint32_t)n, (almond_dirent_t *)(void *)to);
}

int klseek(almond_filehandle_t *fh, uint64_t offset, int whence) {
  uint64_t base;

  if (fh == NULL || fh->id == 0)
    return EINVAL;

  switch (whence) {
  case 0: // SEEK_SET
    base = 0;
    break;
  case 1: // SEEK_CUR
    base = fh->pos;
    break;
  default:
    // SEEK_END needs the driver's file size, which is not part of the
    // generic handle yet.
    return EINVAL;
  }

  if (offset > (~(uint64_t)0) - base)
    return EVFLOW;
  fh->pos = base + offset;
  return 0;
}

int kstat(almond_stat_t *out, const char *path) {
  const char *fs_path;
  partition_info_t *part;
  almond_stat_t tmp;
  int drive;
  int rc;
  uint32_t i;

  if (out == NULL || path == NULL) {
    message_send_message("ERR:part: stat EINVAL\n");
    return EINVAL;
  }

  drive = part_drive_from_path(path, &fs_path);
  if (drive >= 0) {
    if (drive >= PART_DRIVE_COUNT)
      return ENOENT;
    part = &part_db[drive];
    if (!part->present || part->stat == NULL) {
      message_send_message("ERR: part: stat no such partition\n");
      return ENOENT;
    }

    tmp.fsid = part_fsid_for(part);
    rc = part->stat(&tmp, fs_path);
    if (rc == 0)
      *out = tmp;
    else
      message_send_message("ERR: part: stat could not stat\n");
    return rc;
  }

  rc = ENOENT;
  for (i = 0; i < PART_DRIVE_COUNT; ++i) {
    part = &part_db[i];
    if (!part->present || part->stat == NULL)
      continue;

    tmp.fsid = part_fsid_for(part);
    rc = part->stat(&tmp, path);
    if (rc == 0) {
      *out = tmp;
      return 0;
    }
  }
  if (rc == ENOENT)
    message_send_message(" part: could not stat the file\n");
  return rc;
}

static bool_t part_type_is(const char *type, const char *want) {
  if (type == NULL || want == NULL)
    return false;

  while (*type != '\0' && *want != '\0') {
    char a = *type++;
    char b = *want++;

    if (a >= 'A' && a <= 'Z')
      a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z')
      b = (char)(b - 'A' + 'a');
    if (a != b)
      return false;
  }
  return *type == '\0' && *want == '\0';
}

static int part_find_drive(disk_info_t *disk, uint8_t part_n) {
  int i;

  for (i = 0; i < PART_DRIVE_COUNT; ++i) {
    if (part_db[i].present && part_db[i].parent == disk &&
        part_db[i].part_n == part_n)
      return i;
  }
  return -1;
}

static int part_free_drive(void) {
  int i;

  for (i = 0; i < PART_DRIVE_COUNT; ++i) {
    if (!part_db[i].present)
      return i;
  }
  return -1;
}

// Accept either a drive index (0-25) or a letter ('A'-'Z' / 'a'-'z').
static int part_drive_slot(char drive) {
  unsigned char d = (unsigned char)drive;

  if (d >= 'A' && d <= 'Z')
    return d - 'A';
  if (d >= 'a' && d <= 'z')
    return d - 'a';
  if (d < PART_DRIVE_COUNT)
    return (int)d;
  return -1;
}

static void part_set_drive_name(int slot) {
  part_db[slot].pretty_name[0] = (char)('A' + slot);
  part_db[slot].pretty_name[1] = ':';
  part_db[slot].pretty_name[2] = '\0';
}

// Shared mount path.  Returns 0 on success, -1 if the requested drive slot is
// unavailable, or -2 if the filesystem could not be mounted.
static int part_mount_slot(int slot, disk_info_t *disk, uint8_t part_n,
                           char *parttype) {
  partition_info_t zero = {0};

  if (slot < 0 || slot >= PART_DRIVE_COUNT)
    return PART_MOUNT_UNAVAILABLE;

  if (part_db[slot].present) {
    // Already mounted in this slot as the same partition: treat as success.
    if (part_db[slot].parent == disk && part_db[slot].part_n == part_n)
      return 0;
    return PART_MOUNT_UNAVAILABLE;
  }

  if (!part_type_is(parttype, "ext2"))
    return PART_MOUNT_FAILED;

  part_db[slot] = zero;
  if (!ext2_mount_part(&part_db[slot], disk, part_n)) {
    part_db[slot] = zero;
    return PART_MOUNT_FAILED;
  }

  part_set_drive_name(slot);
  return 0;
}

// Mount a partition using the named filesystem driver, assigning the first
// free drive letter.
int part_mount_g(disk_info_t *disk, char part_n, char *parttype) {
  uint8_t pn = (uint8_t)part_n;
  int slot;
  int rc;

  if (disk == NULL || parttype == NULL)
    return EINVAL;
  if (!disk->present || !disk->is_mbr || disk->read == NULL)
    return ENOENT;
  if (pn >= 4 || disk->partition_metadata[pn].type == 0)
    return ENOENT;

  if (!part_type_is(parttype, "ext2"))
    return EINVAL;

  // Mounting the same partition twice is a no-op; a partition keeps its drive
  // letter until it is unmounted.
  slot = part_find_drive(disk, pn);
  if (slot >= 0)
    return 0;

  slot = part_free_drive();
  if (slot < 0)
    return ENOSPC;

  rc = part_mount_slot(slot, disk, pn, parttype);
  return rc == 0 ? 0 : EINVAL;
}

// Mount a partition into a specific drive slot.  The drive may be a letter
// ('A'-'Z' / 'a'-'z') or an index (0-25).
// Returns 0 on success, -1 if the drive is unavailable, and -2 if mounting
// failed.
int part_mount_drive(disk_info_t *disk, char part_n, char drive,
                     char *parttype) {
  uint8_t pn = (uint8_t)part_n;
  int slot;

  if (disk == NULL || parttype == NULL)
    return PART_MOUNT_UNAVAILABLE;
  if (!disk->present || !disk->is_mbr || disk->read == NULL)
    return PART_MOUNT_UNAVAILABLE;
  if (pn >= 4 || disk->partition_metadata[pn].type == 0)
    return PART_MOUNT_UNAVAILABLE;

  slot = part_drive_slot(drive);
  if (slot < 0)
    return PART_MOUNT_UNAVAILABLE;

  return part_mount_slot(slot, disk, pn, parttype);
}

void part_init(void) {
  partition_info_t zero = {0};
  uint32_t i;

  for (i = 0; i < PART_DRIVE_COUNT; ++i)
    part_db[i] = zero;
  for (i = 0; i < PART_WORDS; ++i)
    part_bitmap[i] = 0;
  part_next = 0;
}
