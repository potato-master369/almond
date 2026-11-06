// keymap.c
// ----------------------------------------
// Handle console keymaps

#include "messaging/messaging.h"
#include "disk/part/part.h"
#include "mmu/kmalloc.h"

#include "keymap.h"
// Basically, ts file provides a simple api
//
// -> init
//    -> read file C:/Almond/SYSTEM/cur.akm
//    -> Load as an array (cast)
//    -> Check magic
//    -> if any of the above fail, load default embedded keymap
//
// -> Simple function for converting keystrokes (HID) to ASCII.
//    Currently, UTF not supported!! 申し訳ございません。

static almond_keymap_t *cur_kmap;
extern almond_keymap_t fallback_keymap;

// Fallback - load built-in keymap
static void _keymap_init_fallback(void) {
  cur_kmap = &fallback_keymap;
}

void keymap_init(void) {
  message_send_message(" keymap: initialising console keymaps...\n");
  almond_stat_t st;
  almond_filehandle_t fh;
  message_send_message(" keymap: Loading C:/Almond/SYSTEM/cur.akm...\n");
  if (kstat(&st, "C:/Almond/SYSTEM/cur.akm") != 0) {
	  _keymap_init_fallback(); // check if the file exists
	  return;
  }
  almond_keymap_t *k = kmalloc(st.size + 1);
  if (kopen(&fh, "C:/Almond/SYSTEM/cur.akm", "r") != 0) {
    kfree(k);
    message_send_message("WARN:keymap: could not open C:/Almond/SYSTEM/cur.akm\n");
    _keymap_init_fallback();
    return;
  }
  if (kread(&fh, k, st.size) < 0) {
    kfree(k);
    kclose(&fh);
    message_send_message("WARN:keymap: could not read C:/Almond/SYSTEM/cur.akm\n");
    _keymap_init_fallback();
    return;
  }

  if (k->magic[0] != 'a' || k->magic[1] != 'k' || k->magic[2] != 'm' || k->magic[3] != 'p' || k->len <= 0) {
    kfree(k);
    kclose(&fh);
    message_send_message("WARN:keymap: C:/Almond/SYSTEM/cur.akm is NOT a valid keymap.\n");
    _keymap_init_fallback();
    return;
  }

  cur_kmap = k; // will never be freed cos memory will be used until shutdown
  message_send_message(" keymap: C:/Almond/SYSTEM/cur.akm successfully loaded!\n");

  kclose(&fh);
  return;
}

char keymap_convert_to_ascii(uint8_t hid, bool_t caps, bool_t shift, bool_t altgr) {
  uint16_t shifted = ((caps & 0x01) << 8) | 
                   ((shift & 0x01) << 9) | 
                   ((altgr & 0x01) << 10) | 
                   hid;
  for (int i = 0; i < cur_kmap->len; ++i) {
    if (cur_kmap->data[i].key == shifted) {
      return (char)cur_kmap->data[i].value;
    }
  }
  return -1;
}
