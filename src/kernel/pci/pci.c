// pci.c
// --------------------------
// Scan PCI driver for PCI operations

#include "pci.h"
#include "../helpers/string.h"
#include "../kernel_ports.h"
#include "../kernel_types.h"
#include "../messaging/messaging.h"


// include drivers dat need PCI here
// こちらがPCIを必要とするドライバーです。
#include "../disk/drv/ide.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// code from OSDev wiki
// source: https://wiki.osdev.org/PCI
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func,
                              uint8_t offset) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;
  uint16_t tmp = 0;

  // Create configuration address as per Figure 1
  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((uint32_t)0x80000000));

  // Write out the address
  outl(0xCF8, address);
  // Read in the data
  // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
  tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
  return tmp;
}

uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func,
                               uint8_t offset) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;

  // Create configuration address as per the PCI specification
  // Ensure the offset is 32-bit aligned by clearing the lower 2 bits
  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((uint32_t)0x80000000));

  // Write out the address to the PCI index port
  outl(0xCF8, address);

  // Read back the full 32-bit DWORD from the data port
  return inl(0xCFC);
}

void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func,
                            uint8_t offset, uint16_t val) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;

  // Create configuration address as per the PCI specification
  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((uint32_t)0x80000000));

  // Write out the address to the PCI index port
  outl(0xCF8, address);
  
  // Write the 16-bit word to the data port, offset by 0 or 2 bytes
  outw(0xCFC + (offset & 2), val);
}

void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset, uint32_t val) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;

  // Create configuration address as per the PCI specification
  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((uint32_t)0x80000000));

  // Write out the address to the PCI index port
  outl(0xCF8, address);

  // Write the full 32-bit DWORD to the data port
  outl(0xCFC, val);
}

void pci_check_func(uint8_t bus, uint8_t slot, uint8_t func) {
  char reportbuf[256];
  char messagebuf[256];
  uint16_t vendor_id = pci_config_read_word(bus, slot, func, 0x00);
  if (vendor_id == 0xFFFF)
    return;
  uint16_t device_id = pci_config_read_word(bus, slot, func, 0x02);
  uint16_t class_word = pci_config_read_word(bus, slot, func, 0x0A);
  uint8_t subclass = (class_word >> 0) & 0xFF;
  uint8_t subclass_code = class_word & 0xFF;
  uint8_t class_code = (class_word >> 8) & 0xFF;
  messagebuf[0] = '\0';
  append_string(messagebuf, " pci: found ", sizeof(messagebuf));
  // print what it actually is as a human readable string
  switch (class_code) {
  case 0x00:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "Non-VGA-Compatible Unclassified Device");
      break;
    case 0x01:
      strcpy(reportbuf, "VGA-Compatible Unclassified Device");
      break;
    default:
      strcpy(reportbuf, "Unclassified Device");
      break;
    }
    break;
  case 0x01:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "SCSI Bus Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "IDE Controller");
      // probe IDE driver
      break;
    case 0x02:
      strcpy(reportbuf, "Floppy Disk Controller");
      break;
    case 0x03:
      strcpy(reportbuf, "IPI Bus Controller");
      break;
    case 0x04:
      strcpy(reportbuf, "RAID Controller");
      break;
    case 0x05:
      strcpy(reportbuf, "ATA Controller");
      break;
    case 0x06:
      strcpy(reportbuf, "Serial ATA Controller");
      break;
    case 0x07:
      strcpy(reportbuf, "Serial Attached SCSI Controller");
      break;
    case 0x08:
      strcpy(reportbuf, "Non-Volatile Memory Controller");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Mass Storage Controller");
      break;
    default:
      strcpy(reportbuf, "Mass Storage Controller");
      break;
    }
    break;
  case 0x02:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "Ethernet Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "Token Ring Controller");
      break;
    case 0x02:
      strcpy(reportbuf, "FDDI Controller");
      break;
    case 0x03:
      strcpy(reportbuf, "ATM Controller");
      break;
    case 0x04:
      strcpy(reportbuf, "ISDN Controller");
      break;
    case 0x05:
      strcpy(reportbuf, "WorldFip Controller");
      break;
    case 0x06:
      strcpy(reportbuf, "PICMG 2.14 Multi Computing Controller");
      break;
    case 0x07:
      strcpy(reportbuf, "Infiniband Controller");
      break;
    case 0x08:
      strcpy(reportbuf, "Fabric Controller");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Network Controller");
      break;
    default:
      strcpy(reportbuf, "Network Controller");
      break;
    }
    break;
  case 0x03:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "VGA Compatible Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "XGA Controller");
      break;
    case 0x02:
      strcpy(reportbuf, "3D Controller (Not VGA-Compatible)");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Display Controller");
      break;
    default:
      strcpy(reportbuf, "Display Controller");
      break;
    }
    break;
  case 0x04:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "Multimedia Video Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "Multimedia Audio Controller");
      break;
    case 0x02:
      strcpy(reportbuf, "Computer Telephony Device");
      break;
    case 0x03:
      strcpy(reportbuf, "Audio Device");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Multimedia Controller");
      break;
    default:
      strcpy(reportbuf, "Multimedia Controller");
      break;
    }
    break;
  case 0x05:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "RAM Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "Flash Controller");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Memory Controller");
      break;
    default:
      strcpy(reportbuf, "Memory Controller");
      break;
    }
    break;
  case 0x06:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "Host Bridge");
      break;
    case 0x01:
      strcpy(reportbuf, "ISA Bridge");
      break;
    case 0x02:
      strcpy(reportbuf, "EISA Bridge");
      break;
    case 0x03:
      strcpy(reportbuf, "MCA Bridge");
      break;
    case 0x04:
      strcpy(reportbuf, "PCI-to-PCI Bridge");
      break;
    case 0x05:
      strcpy(reportbuf, "PCMCIA Bridge");
      break;
    case 0x06:
      strcpy(reportbuf, "NuBus Bridge");
      break;
    case 0x07:
      strcpy(reportbuf, "CardBus Bridge");
      break;
    case 0x08:
      strcpy(reportbuf, "RACEway Bridge");
      break;
    case 0x09:
      strcpy(reportbuf, "PCI-to-PCI Bridge");
      break;
    case 0x0A:
      strcpy(reportbuf, "InfiniBand-to-PCI Host Bridge");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Bridge");
      break;
    default:
      strcpy(reportbuf, "Bridge");
      break;
    }
    break;
  case 0x07:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "Serial Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "Parallel Controller");
      break;
    case 0x02:
      strcpy(reportbuf, "Multiport Serial Controller");
      break;
    case 0x03:
      strcpy(reportbuf, "Modem");
      break;
    case 0x04:
      strcpy(reportbuf, "IEEE 488.1/2 (GPIB) Controller");
      break;
    case 0x05:
      strcpy(reportbuf, "Smart Card Controller");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Simple Communication Controller");
      break;
    default:
      strcpy(reportbuf, "Simple Communication Controller");
      break;
    }
    break;
  case 0x08:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "PIC");
      break;
    case 0x01:
      strcpy(reportbuf, "DMA Controller");
      break;
    case 0x02:
      strcpy(reportbuf, "Timer");
      break;
    case 0x03:
      strcpy(reportbuf, "RTC Controller");
      break;
    case 0x04:
      strcpy(reportbuf, "PCI Hot-Plug Controller");
      break;
    case 0x05:
      strcpy(reportbuf, "SD Host controller");
      break;
    case 0x06:
      strcpy(reportbuf, "IOMMU");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Base System Peripheral");
      break;
    default:
      strcpy(reportbuf, "Base System Peripheral");
      break;
    }
    break;
  case 0x09:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "Keyboard Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "Digitizer Pen");
      break;
    case 0x02:
      strcpy(reportbuf, "Mouse Controller");
      break;
    case 0x03:
      strcpy(reportbuf, "Scanner Controller");
      break;
    case 0x04:
      strcpy(reportbuf, "Gameport Controller");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Input Device Controller");
      break;
    default:
      strcpy(reportbuf, "Input Device Controller");
      break;
    }
    break;
  case 0x0A:
    strcpy(reportbuf, "Docking Station");
    break;
  case 0x0B:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "386");
      break;
    case 0x01:
      strcpy(reportbuf, "486");
      break;
    case 0x02:
      strcpy(reportbuf, "Pentium");
      break;
    case 0x03:
      strcpy(reportbuf, "Pentium Pro");
      break;
    case 0x10:
      strcpy(reportbuf, "Alpha");
      break;
    case 0x20:
      strcpy(reportbuf, "PowerPC");
      break;
    case 0x30:
      strcpy(reportbuf, "MIPS");
      break;
    case 0x40:
      strcpy(reportbuf, "Co-Processor");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Processor");
      break;
    default:
      strcpy(reportbuf, "Processor");
      break;
    }
    break;
  case 0x0C:
    switch (subclass_code) {
    case 0x00:
      strcpy(reportbuf, "FireWire (IEEE 1394) Controller");
      break;
    case 0x01:
      strcpy(reportbuf, "ACCESS Bus Controller");
      break;
    case 0x02:
      strcpy(reportbuf, "SSA");
      break;
    case 0x03:
      strcpy(reportbuf, "USB Controller");
      break;
    case 0x04:
      strcpy(reportbuf, "Fibre Channel");
      break;
    case 0x05:
      strcpy(reportbuf, "SMBus Controller");
      break;
    case 0x06:
      strcpy(reportbuf, "InfiniBand Controller");
      break;
    case 0x07:
      strcpy(reportbuf, "IPMI Interface");
      break;
    case 0x08:
      strcpy(reportbuf, "SERCOS Interface (IEC 61491)");
      break;
    case 0x09:
      strcpy(reportbuf, "CANbus Controller");
      break;
    case 0x80:
      strcpy(reportbuf, "Other Serial Bus Controller");
      break;
    default:
      strcpy(reportbuf, "Serial Bus Controller");
      break;
    }
    break;
  case 0x0D:
    strcpy(reportbuf, "Wireless Controller");
    break;
  case 0x0E:
    strcpy(reportbuf, "Intelligent Controller");
    break;
  case 0x0F:
    strcpy(reportbuf, "Satellite Communication Controller");
    break;
  case 0x10:
    strcpy(reportbuf, "Encryption Controller");
    break;
  case 0x11:
    strcpy(reportbuf, "Signal Processing Controller");
    break;
  case 0x12:
    strcpy(reportbuf, "Processing Accelerator");
    break;
  case 0x13:
    strcpy(reportbuf, "Non-Essential Instrumentation");
    break;
  case 0x40:
    strcpy(reportbuf, "Co-Processor");
    break;
  case 0xFF:
    strcpy(reportbuf, "Unassigned Class (Vendor specific)");
    break;
  default:
    strcpy(reportbuf, "Unknown PCI Device");
    break;
  }
  append_string(messagebuf, reportbuf, sizeof(messagebuf));
  append_string(messagebuf, " on ", sizeof(messagebuf));
  itoa_hex(bus, reportbuf);
  append_string(messagebuf, reportbuf, sizeof(messagebuf));
  append_string(messagebuf, "/", sizeof(messagebuf));
  itoa_hex(slot, reportbuf);
  append_string(messagebuf, reportbuf, sizeof(messagebuf));
  append_string(messagebuf, "/", sizeof(messagebuf));
  itoa_hex(func, reportbuf);
  append_string(messagebuf, reportbuf, sizeof(messagebuf));
  append_string(messagebuf, "\n", sizeof(messagebuf));
  message_send_message(messagebuf);
}

// helpers for our drivers :D

uint8_t pci_get_bar_type(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_n) {
  uint32_t bar_raw = pci_config_read_dword(bus, slot, func, 0x10 + 4 * bar_n);
  return (bar_raw >> 1) & 0x03;
}

uint8_t pci_get_bar_is_mmio(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_n) {
  uint32_t bar_raw = pci_config_read_dword(bus, slot, func, 0x10 + 4 * bar_n);
  return (bar_raw) & 1;
}

uint32_t pci_get_bar_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_n) {
  uint32_t bar_raw = pci_config_read_dword(bus, slot, func, 0x10 + 4 * bar_n);
  // from osdev wiki
  if (pci_get_bar_type(bus, slot, func, bar_n) == 0)
    return (bar_raw & 0xFFFFFFF0);
  else
    return (bar_raw & 0xFFFFFFFC);
}

#define PCI_CMD_BUS_MASTER (1 << 2)

void pci_config_set_bus_master(uint8_t bus, uint8_t slot, uint8_t func, bool_t is_master) {
  uint16_t command = pci_config_read_word(bus, slot, func, 0x04);
  command = is_master ? (command | PCI_CMD_BUS_MASTER) : (command & ~PCI_CMD_BUS_MASTER);
  pci_config_write_word(bus, slot, func, 0x04, command);
}

// check a device (ie ensure not null and check functions)
void pci_check_device(uint8_t bus, uint8_t device) {
  uint16_t vendor_id = pci_config_read_word(bus, device, 0, 0x00);
  uint16_t reg_0e = pci_config_read_word(bus, device, 0, 0x0E);
  uint8_t header_type = reg_0e & 0xFF;
  uint8_t is_multifunction = (header_type & 0x80) != 0; // Bit 7
  uint8_t layout_type = header_type & 0x7F;             // Bits 6-0
  if (vendor_id == 0xFFFF)
    return;
  if (!is_multifunction) {
    pci_check_func(bus, device, 0);
    return;
  }
  for (uint8_t func = 0; func < 8; ++func) {
    pci_check_func(bus, device, func);
  }
}

void pci_init(void) {
  message_send_message(" pci: init\n");
  for (uint16_t bus = 0; bus < 256; ++bus) {
    for (uint8_t slot = 0; slot < 32; ++slot) {
      pci_check_device(bus, slot);
    }
  }
}
