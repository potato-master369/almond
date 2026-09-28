// ide.c
// -------------------
// MIT License; Copyright (C) potato-master369 2026-
// IDE driver for ABL

#include "../bl_ports.h"
#include "../bl_types.h"
#include "../bl_vga.h"
#include "disk.h"

// register mappings
#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CTRL 0x376

#define ATA_REG_DATA 0
#define ATA_REG_ERROR 1
#define ATA_REG_SECCOUNT0 2
#define ATA_REG_LBA0 3
#define ATA_REG_LBA1 4
#define ATA_REG_LBA2 5
#define ATA_REG_HDDEVSEL 6
#define ATA_REG_COMMAND 7
#define ATA_REG_STATUS 7

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_SR_ERR 0x01
#define ATA_SR_DRQ 0x08
#define ATA_SR_BSY 0x80

#define DRV_ID_IDE 0x0001

// helpers
static inline uint16_t ide_io_base(uint8_t drv_id) {
  return (drv_id & 0x02) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
}

static inline uint16_t ide_ctrl_base(uint8_t drv_id) {
  return (drv_id & 0x02) ? ATA_SECONDARY_CTRL : ATA_PRIMARY_CTRL;
}

static inline uint8_t ide_slave_bit(uint8_t drv_id) { return (drv_id & 0x01); }
static inline void ide_panic(const char *code) {
  bl_vga_write(code, 0x1E);
  for (;;)
    ;
}

// read sector_offset into *buf
uint8_t ide_read(uint32_t sector_offset, void *buf, uint8_t drv_id) {
  uint16_t io = ide_io_base(drv_id);
  uint8_t slave = ide_slave_bit(drv_id);
  uint8_t status;

  while (inb(io + ATA_REG_STATUS) & ATA_SR_BSY)
    ;

  outb(io + ATA_REG_HDDEVSEL,
       0xE0 | (slave << 4) | ((sector_offset >> 24) & 0x0F));

  outb(io + ATA_REG_SECCOUNT0, 1);
  outb(io + ATA_REG_LBA0, (uint8_t)(sector_offset & 0xFF));
  outb(io + ATA_REG_LBA1, (uint8_t)((sector_offset >> 8) & 0xFF));
  outb(io + ATA_REG_LBA2, (uint8_t)((sector_offset >> 16) & 0xFF));
  outb(io + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

  do {
    status = inb(io + ATA_REG_STATUS);
  } while (status & ATA_SR_BSY);

  if (status & ATA_SR_ERR) {
    ide_panic("E7001"); // read error
  }

  while (!(inb(io + ATA_REG_STATUS) & ATA_SR_DRQ))
    ;

  uint16_t *buf16 = (uint16_t *)buf;
  for (int i = 0; i < 256; i++) {
    buf16[i] = inw(io + ATA_REG_DATA);
  }
  return 0;
}

// write to the sector using the pointer at *buf (assume perfect 512 byte
// buffer)
uint8_t ide_write(uint32_t sector_offset, void *buf, uint8_t drv_id) {
  uint16_t io = ide_io_base(drv_id);
  uint8_t slave = ide_slave_bit(drv_id);
  uint8_t status;

  while (inb(io + ATA_REG_STATUS) & ATA_SR_BSY)
    ;

  outb(io + ATA_REG_HDDEVSEL,
       0xE0 | (slave << 4) | ((sector_offset >> 24) & 0x0F));

  outb(io + ATA_REG_SECCOUNT0, 1);
  outb(io + ATA_REG_LBA0, (uint8_t)(sector_offset & 0xFF));
  outb(io + ATA_REG_LBA1, (uint8_t)((sector_offset >> 8) & 0xFF));
  outb(io + ATA_REG_LBA2, (uint8_t)((sector_offset >> 16) & 0xFF));
  outb(io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

  do {
    status = inb(io + ATA_REG_STATUS);
  } while (status & ATA_SR_BSY);

  if (status & ATA_SR_ERR) {
    ide_panic("E7002"); // write error
  }

  while (!(inb(io + ATA_REG_STATUS) & ATA_SR_DRQ))
    ;

  uint16_t *buf16 = (uint16_t *)buf;
  for (int i = 0; i < 256; i++) {
    outw(io + ATA_REG_DATA, buf16[i]);
  }

  outb(io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
  while (inb(io + ATA_REG_STATUS) & ATA_SR_BSY)
    ;
  return 0;
}

// discover IDE disks
void ide_discover(disk_t **disk_info) {
  bl_vga_write(" ide: populate\n", 0x07);
  // to populate
  // *disk_info[n].drv_id = ((uint16_t)DRV_ID_IDE << 8) | internal disk ID
  // *disk_info[n].read_sec = ide_read
  // *disk_info[n].write_sec = ide_read
  // *disk_info[n].pretty_name = "ideN" e.g. ide0
  int16_t identify_buf[256];
  int found = 0;

  for (uint8_t internal_id = 0; internal_id < 4; internal_id++) {
    uint16_t io = ide_io_base(internal_id);
    uint8_t slave = ide_slave_bit(internal_id);

    // select drive
    outb(io + ATA_REG_HDDEVSEL, 0xA0 | (slave << 4));

    // small delay after drive select (400ns rule of thumb)
    inb(io + ATA_REG_STATUS);
    inb(io + ATA_REG_STATUS);
    inb(io + ATA_REG_STATUS);
    inb(io + ATA_REG_STATUS);

    outb(io + ATA_REG_SECCOUNT0, 0);
    outb(io + ATA_REG_LBA0, 0);
    outb(io + ATA_REG_LBA1, 0);
    outb(io + ATA_REG_LBA2, 0);
    outb(io + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(io + ATA_REG_STATUS);
    if (status == 0) {
      // no drive on this channel/slot at all
      continue;
    }

    // poll until BSY clears
    while ((status = inb(io + ATA_REG_STATUS)) & ATA_SR_BSY)
      ;

    // check for ATAPI/SATA signature instead of a real PATA drive
    uint8_t lba1 = inb(io + ATA_REG_LBA1);
    uint8_t lba2 = inb(io + ATA_REG_LBA2);
    if (lba1 != 0 || lba2 != 0) {
      // not a plain ATA disk - skip (ATAPI etc.)
      continue;
    }

    // wait for either DRQ (data ready) or ERR
    while (!((status = inb(io + ATA_REG_STATUS)) & (ATA_SR_DRQ | ATA_SR_ERR)))
      ;

    if (status & ATA_SR_ERR) {
      continue;
    }

    // drain the 256-word identify block
    for (int i = 0; i < 256; i++) {
      identify_buf[i] = inw(io + ATA_REG_DATA);
    }

    // drive exists
    bl_vga_write(" ide: drive found\n", 0x07);

    // find the first free slot (drv_id == DRV_ID_NULL) to occupy
    int slot = -1;
    for (int i = 0; i < DISK_MAX; i++) {
      if (disk_info[i]->drv_id == DRV_ID_NULL) {
        slot = i;
        break;
      }
    }

    if (slot == -1) {
      // no free slots left in disk_info - can't register this drive
      ide_panic("E0006");
    }

    disk_info[slot]->drv_id = ((uint16_t)DRV_ID_IDE << 8) | internal_id;
    disk_info[slot]->read_sec = ide_read;
    disk_info[slot]->write_sec = ide_write;

    disk_info[slot]->pretty_name[0] = 'i';
    disk_info[slot]->pretty_name[1] = 'd';
    disk_info[slot]->pretty_name[2] = 'e';
    disk_info[slot]->pretty_name[3] = '0' + found;
    disk_info[slot]->pretty_name[4] = '\0';

    found++;
  }
}
