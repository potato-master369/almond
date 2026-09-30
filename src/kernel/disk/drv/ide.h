#ifndef IDE_H
#define IDE_H
#include "../disk.h"
#include "../../kernel_types.h"

typedef struct {
  bool_t dma;
} ide_featuremask_t;
void ide_init(disk_info_t *container);

void ide_pci_probe(uint8_t bus, uint8_t slot, uint8_t func);
#endif
