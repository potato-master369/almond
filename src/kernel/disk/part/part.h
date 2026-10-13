// part.h
// -------------------------------
// From here on, partitions need to present
// an abstraction layer of file handles and
// allat bullshit. 
//
// うまゆるふるげーと is a great series
#ifndef DISK_PART_H
#define DISK_PART_H
#include "../../kernel_types.h"

// defines
#define PART_MAX_OPEN 1024

// File handle
typedef struct {
  uint32_t id;
  uint32_t fsid;
  uint32_t resv;
  uint16_t generation_count;
  uint32_t inode_n;
} almond_filehandle_t;

typedef uint32_t FILE;
#endif
