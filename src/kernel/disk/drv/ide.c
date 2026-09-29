// ide.c
// -----------------------------
// ATA IDE driver for Almond

#include "../../helpers/string.h"
#include "../../kernel_ports.h"
#include "../../kernel_types.h"
#include "../../messaging/messaging.h"
#include "../disk.h"
#include "ide.h"

// settings
ide_featuremask_t ide_featuremask = {.dma = false};

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
static inline uint16_t ide_io_base(uint8_t internal_id) {
  return (internal_id & 0x02) ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
}

static inline uint16_t ide_ctrl_base(uint8_t internal_id) {
  return (internal_id & 0x02) ? ATA_SECONDARY_CTRL : ATA_PRIMARY_CTRL;
}

static inline uint8_t ide_slave_bit(uint8_t drv_id) { return (drv_id & 0x01); }

// standard r/w functions
//  - the else block of !ide_featuremask.dma is for
//    backup PIO read
uint8_t ide_read(disk_info_t *self, uint32_t lba, uint32_t n, void *to) {
  if (lba > self->size || n > self->size - lba)
	  return DISK_ERR_RANGE;
  if (!ide_featuremask.dma) {

  } else {
    uint16_t *buf16 = (uint16_t *)to;
    uint8_t drv_id = self->resv & 0xFF;
    uint16_t io = ide_io_base(drv_id);
    uint8_t slave = ide_slave_bit(drv_id);

    for (uint32_t sector = 0; sector < n; sector++) {
      uint32_t current_lba = lba + sector;
      uint32_t timeout = 100000;
      while ((inb(io + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout)
        ;
      if (timeout == 0)
        return DISK_ERR_HW;

      outb(io + ATA_REG_HDDEVSEL,
           0xE0 | (slave << 4) | ((current_lba >> 24) & 0x0F));
      outb(io + ATA_REG_SECCOUNT0, 1);
      outb(io + ATA_REG_LBA0, (uint8_t)(current_lba & 0xFF));
      outb(io + ATA_REG_LBA1, (uint8_t)((current_lba >> 8) & 0xFF));
      outb(io + ATA_REG_LBA2, (uint8_t)((current_lba >> 16) & 0xFF));
      outb(io + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

      uint8_t status = inb(io + ATA_REG_STATUS);
      timeout = 100000;
      while (!(status & ATA_SR_ERR) &&
             ((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ)) && timeout) {
        status = inb(io + ATA_REG_STATUS);
        --timeout;
      }
      if (timeout == 0 || (status & ATA_SR_ERR) || !(status & ATA_SR_DRQ))
        return DISK_ERR_HW;

      for (int i = 0; i < 256; i++)
        *buf16++ = inw(io + ATA_REG_DATA);
    }
  }
  return 0;
}

uint8_t ide_write(disk_info_t *self, uint32_t lba, uint32_t n, void *from) {
  if (lba > self->size || n > self->size - lba)
	  return DISK_ERR_RANGE;
  if (!ide_featuremask.dma) {

  } else {
    uint16_t *buf16 = (uint16_t *)from;

    uint8_t drv_id = self->resv & 0xFF;
    uint16_t io = ide_io_base(drv_id);
    uint8_t slave = ide_slave_bit(drv_id);
    for (uint32_t sector = 0; sector < n; sector++) {

      uint32_t current_lba = lba + sector;
      // Wait for BSY to clear with a safety timeout
      uint32_t timeout = 100000;
      while ((inb(io + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout)
        ;
      if (timeout == 0)
        return DISK_ERR_HW;

      outb(io + ATA_REG_HDDEVSEL,
           0xE0 | (slave << 4) | ((current_lba >> 24) & 0x0F));
      outb(io + ATA_REG_SECCOUNT0, 1);
      outb(io + ATA_REG_LBA0, (uint8_t)(current_lba & 0xFF));
      outb(io + ATA_REG_LBA1, (uint8_t)((current_lba >> 8) & 0xFF));
      outb(io + ATA_REG_LBA2, (uint8_t)((current_lba >> 16) & 0xFF));

      outb(io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

      timeout = 100000;
      uint8_t status;
      while (!(((status = inb(io + ATA_REG_STATUS)) &
                (ATA_SR_DRQ | ATA_SR_ERR))) &&
             --timeout)
        ;

      if (timeout == 0 || (status & ATA_SR_ERR)) {
        return DISK_ERR_HW;
      }

      for (int i = 0; i < 256; i++) {
        outw(io + ATA_REG_DATA, *buf16++);
      }
    }

    outb(io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    uint32_t timeout = 100000;
    while ((inb(io + ATA_REG_STATUS) & ATA_SR_BSY) && --timeout)
      ;
    if (timeout == 0)
      return DISK_ERR_HW;
  }
  return 0;
}

void ide_init(disk_info_t *container) {
  message_send_message(" ide: discover\n");
  // copied over from boot ide driver, modified
  uint16_t identity_buf[256];
  int found = 0;
  for (uint8_t internal_id = 0; internal_id < 4; ++internal_id) {
    uint16_t io = ide_io_base(internal_id);
    uint8_t slave = (internal_id & 0x01);

    outb(io + ATA_REG_HDDEVSEL, 0xA0 | (slave << 4));
    inb(io + ATA_REG_STATUS);
    inb(io + ATA_REG_STATUS);
    inb(io + ATA_REG_STATUS);
    inb(io + ATA_REG_STATUS);

    outb(io + ATA_REG_SECCOUNT0, 0);
    outb(io + ATA_REG_LBA0, 0);
    outb(io + ATA_REG_LBA1, 0);
    outb(io + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(io + ATA_REG_STATUS);
    if (status == 0) {
      // no drive on this channel/slot
      continue;
    }

    while ((status = inb(io + ATA_REG_STATUS)) & ATA_SR_BSY)
      ;

    uint8_t lba1 = inb(io + ATA_REG_LBA1);
    uint8_t lba2 = inb(io + ATA_REG_LBA2);
    if (lba1 != 0 || lba2 != 0) {
      continue; // not plain ATA - skip
    }
    uint32_t total_sectors = ((uint32_t)identity_buf[61] << 16) | identity_buf[60];

    // wait for DRQ or ERR
    while (!((status = inb(io + ATA_REG_STATUS)) & (ATA_SR_DRQ | ATA_SR_ERR)))
      ;

    if (status & ATA_SR_ERR) {
      continue;
    }

    message_send_message(" ide: found!\n");

    int slot = -1;
    for (int i = 0; i < DISK_MAX; ++i) {
      if (container[i].present == 0) {
        slot = i;
        break;
      }
    }

    if (slot == -1)
      return; // just pretend we didnt see shit

    container[slot].resv = ((uint16_t)DRV_ID_IDE << 8) | internal_id;
    container[slot].read = ide_read;
    container[slot].write = ide_write;
    container[slot].size = total_sectors;
    container[slot].pretty_name[0] = 'i';
    container[slot].pretty_name[1] = 'd';
    container[slot].pretty_name[2] = 'e';
    container[slot].pretty_name[3] = '0' + found;
    container[slot].pretty_name[4] = '\0';
  }
}
