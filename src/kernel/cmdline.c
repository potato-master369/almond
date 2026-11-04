// cmdline.c
// ----------------------------
// Handler for cmdline of kernel
// kek
#include "messaging/messaging.h"
#include "helpers/string.h"
#include "disk/disk.h"
#include "disk/drv/ide.h"

#include "cmdline.h"

extern disk_featuremask_t disk_drivermasks;
extern ide_featuremask_t ide_featuremask;
extern core_featuremask_t core_featuremask;
extern char rootdisk[64];
extern char rootfstype[64];
uint16_t checkmask;

void cmdline_parse_cmdline(const char *cmdline) {
  int p,s;
  char wordbuf[64]; // kmalloc not active yet?
  checkmask = 0;
  s = 0;
  while (cmdline[s] != '\0') {
    p = 0;
    while (cmdline[s] != ' ' && cmdline[s] != '\0' && p < 63) {
      wordbuf[p] = cmdline[s];
      ++p;
      ++s;
    }
    wordbuf[p] = '\0'; // null-terminate
    // parse
    message_send_message(" cmdline: word \"");
    message_send_message(wordbuf);
    message_send_message("\"\n");
    if (strncmp(wordbuf, "root=", 5) == 0) {
      strcpy(rootdisk, wordbuf + 5);
      checkmask |= (1U << 0);
    } else if (strncmp(wordbuf, "rootfs_type=", 12) == 0) {
      strcpy(rootfstype, wordbuf + 12);
      checkmask |= (1U << 1);
    } else if (strncmp(wordbuf, "-ide.dma", sizeof(wordbuf)) == 0) {
      ide_featuremask.dma = true;
    } else if (strncmp(wordbuf, "-ide", sizeof(wordbuf)) == 0) {
      disk_drivermasks.ide = true;
    } else if (strncmp(wordbuf, "-bsman", sizeof(wordbuf)) == 0) {
      core_featuremask.bsman = true;
    } else if (strncmp(wordbuf, "gvt", sizeof(wordbuf)) == 0) {
      core_featuremask.gvt = false;
      core_featuremask.bsman = true; // gvt implies -bsman
    } else {
      char msgbuf[256] = "WARN:cmdline: unrecognised option: ";
      append_string(msgbuf, wordbuf, sizeof(msgbuf));
      append_string(msgbuf, "\n", sizeof(msgbuf));
      message_send_message(msgbuf);
    }
    ++s;
  }
}
