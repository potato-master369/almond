#ifndef PCI_H
#define PCI_H
#include "../kernel_types.h"
void pci_init(void);
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val);
void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
// helpers
uint32_t pci_get_bar_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_n);
uint8_t pci_get_bar_type(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_n);
uint8_t pci_get_bar_is_mmio(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_n);
void pci_config_set_bus_master(uint8_t bus, uint8_t slot, uint8_t func, bool_t is_master);
#endif
