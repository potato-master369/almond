// small set of helpers to help with parsing the root disk.

#ifndef ALMOND_ROOTDISK_H
#define ALMOND_ROOTDISK_H

#include "kernel_types.h"
#include "helpers/string.h"

static inline bool_t rootdisk_parse(const char *src, char *prettyname, size_t sizeof_prettyname, uint32_t *partition) {
  // validate that its a proper format
  const char *comma = strchr(src, ',');
  if (!comma || comma == src || !comma[1])
	  return false;
  size_t name_len = comma - src;

  if (name_len > sizeof_prettyname)
	  return false;

  // Validate the name
  for (size_t i = 0; i < name_len; ++i) {
#define c src[i]
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
	    return false;
#undef c
  }

  uint32_t part = 0;

  for (const char *p = comma + 1; *p; ++p) {
    if (*p < '0' || *p > '9')
	    return false;
    uint32_t digit = *p - '0';

    if (part > (UINT32_MAX - digit) / 10)
	    return false;

    part = part * 10 + digit;
  }

  memcpy(prettyname, src, name_len);
  prettyname[name_len] = '\0';
  *partition = part;
  return true;
}
#endif
