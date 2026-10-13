#include "config.h"
#include "bl_types.h"
#include "bl_vga.h"
#include "string.h"

boot_conf_t conf = {
  .cmdline = "",
  .prettyname = "ABL Default Config",
  .kernpath = "/boot/almond.bin"
}; // extern this
#define CONFIG_ADDR 0x100000

static int config_key_matches(const char *key, const char *expected) {
  while (*key != '\0' && *expected != '\0' && *key == *expected) {
    ++key;
    ++expected;
  }
  return *key == '\0' && *expected == '\0';
}

static void config_set_value(char *destination, uint32_t capacity,
                             const char *value, uint32_t length) {
  if (length >= 2 && value[0] == '"' && value[length - 1] == '"') {
    ++value;
    length -= 2;
  }
  if (length >= capacity)
    length = capacity - 1;
  memcpy(destination, value, length);
  destination[length] = '\0';
}

static void config_parse_line(char *line) {
  char *key = line;
  while (*key == ' ' || *key == '\t' || *key == '\r')
    ++key;
  if (*key == '\0' || *key == ';' || *key == '#' || *key == '[')
    return;

  char *separator = key;
  while (*separator != '\0' && *separator != '=')
    ++separator;
  if (*separator != '=')
    return;

  char *key_end = separator;
  while (key_end > key &&
         (key_end[-1] == ' ' || key_end[-1] == '\t'))
    --key_end;
  *key_end = '\0';

  char *value = separator + 1;
  while (*value == ' ' || *value == '\t')
    ++value;
  uint32_t length = 0;
  while (value[length] != '\0' && value[length] != '\r')
    ++length;
  while (length > 0 &&
         (value[length - 1] == ' ' || value[length - 1] == '\t'))
    --length;

  if (config_key_matches(key, "ABL_PN")) {
    config_set_value(conf.prettyname, sizeof(conf.prettyname), value, length);
  } else if (config_key_matches(key, "ABL_KP")) {
    config_set_value(conf.kernpath, sizeof(conf.kernpath), value, length);
  } else if (config_key_matches(key, "ABL_CL")) {
    config_set_value(conf.cmdline, sizeof(conf.cmdline), value, length);
  }
}

void config_read_conf(uint32_t config_len) {
  const char *config_buf = (const char *)CONFIG_ADDR;
  char linebuf[256];
  uint32_t line_length = 0;
  int line_too_long = 0;

  for (uint32_t i = 0; i <= config_len; ++i) {
    char character = i < config_len ? config_buf[i] : '\n';
    if (character == '\n' || character == '\0') {
      if (line_too_long) {
        bl_vga_write(" config: line exceeds max line length; abandoning\n",
                     0x04);
      } else {
        linebuf[line_length] = '\0';
        config_parse_line(linebuf);
      }
      line_length = 0;
      line_too_long = 0;
    } else if (!line_too_long) {
      if (line_length < sizeof(linebuf) - 1) {
        linebuf[line_length++] = character;
      } else {
        line_too_long = 1;
      }
    }
  }
}
