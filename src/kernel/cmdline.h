#ifndef CMDLINE_H
#define CMDLINE_H
#include "kernel_types.h"

typedef struct {
  bool_t bsman;
} core_featuremask_t;

void cmdline_parse_cmdline(const char *cmdline);
#endif
