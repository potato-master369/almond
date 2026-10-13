#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H
#include "bl_types.h"
typedef struct {
  char cmdline[256];
  char prettyname[64];
  char kernpath[256];
} boot_conf_t;
extern boot_conf_t conf;
void config_read_conf(uint32_t config_len);
#endif
