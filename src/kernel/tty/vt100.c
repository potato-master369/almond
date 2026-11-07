// vt100.c
// ---------------------------
// stupid vt100 ish driver

#include "framebuffer/fb.h"
#include "kernel_types.h"
#include "messaging/messaging.h"

#define VT100_MAX_PARAMS 8

#define VT100_BLACK 0x000000FFul
#define VT100_RED 0xAA0000FFul
#define VT100_GREEN 0x00AA00FFul
#define VT100_YELLOW 0xAA5500FFul
#define VT100_BLUE 0x0000AAFFul
#define VT100_MAGENTA 0xAA00AAFFul
#define VT100_CYAN 0x00AAAAFFul
#define VT100_WHITE 0xAAAAAAFFul
#define VT100_BRIGHT_BLACK 0x555555FFul
#define VT100_BRIGHT_RED 0xFF5555FFul
#define VT100_BRIGHT_GREEN 0x55FF55FFul
#define VT100_BRIGHT_YELLOW 0xFFFF55FFul
#define VT100_BRIGHT_BLUE 0x5555FFFFul
#define VT100_BRIGHT_MAGENTA 0xFF55FFFFul
#define VT100_BRIGHT_CYAN 0x55FFFFFFul
#define VT100_BRIGHT_WHITE 0xFFFFFFFFul

extern fb_surface_t fb0;
extern const uint8_t terminus_fon[128][16];

struct {
  uint32_t col;
  uint32_t bgcol;
  int32_t x;
  int32_t y;
  bool_t underline;
  bool_t strike;
  bool_t revideo;
} vt100_context;

static void vt100_reset(void) {
  vt100_context.col = 0xAAAAAAFF;
  vt100_context.bgcol = 0x000000FF;
  vt100_context.underline = false;
  vt100_context.strike = false;
  vt100_context.revideo = false;
}

void vt100_init(void) {
  message_send_message(" vt100: init\n");
  vt100_reset();
  fb0.clear(0x000000FF, &fb0);
  vt100_context.x = 0;
  vt100_context.y = 0;
  fb0.update_fb(&fb0);
  message_send_message(" vt100: init success\n");
  fb0.update_fb(&fb0);
}

static void vt100_write_char(char c) {
  for (uint16_t y = 0; y < 16; ++y) {
    uint8_t row = terminus_fon[(unsigned char)c][y];
    for (uint16_t x = 0; x < 8; ++x) {
      fb0.write_pixel(
          (uint32_t)(vt100_context.x + x), (uint32_t)(vt100_context.y + y),
          (row & (1 << x)) ? vt100_context.col : vt100_context.bgcol, &fb0);
    }
  }
  fb0.update_fb(&fb0);
}

static inline void vt100_newline(void) {
  vt100_context.x = 0;

  if (vt100_context.y + 16 >= (int32_t)fb0.height) {
    fb0.scroll_up(16, vt100_context.bgcol, &fb0);
    vt100_context.y = (fb0.height >= 16) ? fb0.height - 16 : 0;
  } else {
    vt100_context.y += 16;
  }
}

static inline void vt100_clamp_cursor(void) {
  int32_t max_x = (fb0.width > 8) ? (int32_t)fb0.width - 8 : 0;
  int32_t max_y = (fb0.height > 16) ? (int32_t)fb0.height - 16 : 0;

  if (vt100_context.x < 0)
    vt100_context.x = 0;
  else if (vt100_context.x > max_x)
    vt100_context.x = max_x;

  if (vt100_context.y < 0)
    vt100_context.y = 0;
  else if (vt100_context.y > max_y)
    vt100_context.y = max_y;
}

static inline void vt100_attempt_m(int x, int y) {
  vt100_context.x += x;
  vt100_context.y += y;
  vt100_clamp_cursor();
}

static inline void vt100_attempt_s(int x, int y) {
  vt100_context.x = x;
  vt100_context.y = y;
  vt100_clamp_cursor();
}

static inline void vt100_set_col(uint32_t col) {
  if (vt100_context.revideo)
    vt100_context.bgcol = col;
  else
    vt100_context.col = col;
}

static inline void vt100_set_bgcol(uint32_t col) {
  if (vt100_context.revideo)
    vt100_context.col = col;
  else
    vt100_context.bgcol = col;
}

static inline uint32_t vt100_xterm_col(int col) {
  if (col < 0 || col >= 256)
    return 0x000000FFul;
  const uint32_t xterm_palette[256] = {
      // 0 - 15: VT100 System Colors
      0x000000FFul, 0xAA0000FFul, 0x00AA00FFul, 0xAA5500FFul, 0x0000AAFFul,
      0xAA00AAFFul, 0x00AAAAFFul, 0xAAAAAAFFul, 0x555555FFul, 0xFF5555FFul,
      0x55FF55FFul, 0xFFFF55FFul, 0x5555FFFFul, 0xFF55FFFFul, 0x55FFFFFFul,
      0xFFFFFFFFul,
      // 16 - 231: 6x6x6 Color Cube
      0x000000FFul, 0x00005FFFul, 0x000087FFul, 0x0000AFFFul, 0x0000D7FFul,
      0x0000FFFFul, 0x005F00FFul, 0x005F5FFFul, 0x005F87FFul, 0x005FAFFFul,
      0x005FD7FFul, 0x005FFFFFul, 0x008700FFul, 0x00875FFFul, 0x008787FFul,
      0x0087AFFFul, 0x0087D7FFul, 0x0087FFFFul, 0x00AF00FFul, 0x00AF5FFFul,
      0x00AF87FFul, 0x00AFAFFFul, 0x00AFD7FFul, 0x00AFFFFFul, 0x00D700FFul,
      0x00D75FFFul, 0x00D787FFul, 0x00D7AFFFul, 0x00D7D7FFul, 0x00D7FFFFul,
      0x00FF00FFul, 0x00FF5FFFul, 0x00FF87FFul, 0x00FFAFFFul, 0x00FFD7FFul,
      0x00FFFFFFul, 0x5F0000FFul, 0x5F005FFFul, 0x5F0087FFul, 0x5F00AFFFul,
      0x5F00D7FFul, 0x5F00FFFFul, 0x5F5F00FFul, 0x5F5F5FFFul, 0x5F5F87FFul,
      0x5F5FAFFFul, 0x5F5FD7FFul, 0x5F5FFFFFul, 0x5F8700FFul, 0x5F875FFFul,
      0x5F8787FFul, 0x5F87AFFFul, 0x5F87D7FFul, 0x5F87FFFFul, 0x5FAF00FFul,
      0x5FAF5FFFul, 0x5FAF87FFul, 0x5FAFAFFFul, 0x5FAFD7FFul, 0x5FAFFFFFul,
      0x5FD700FFul, 0x5FD75FFFul, 0x5FD787FFul, 0x5FD7AFFFul, 0x5FD7D7FFul,
      0x5FD7FFFFul, 0x5FFF00FFul, 0x5FFF5FFFul, 0x5FFF87FFul, 0x5FFFAFFFul,
      0x5FFFD7FFul, 0x5FFFFFFFul, 0x870000FFul, 0x87005FFFul, 0x870087FFul,
      0x8700AFFFul, 0x8700D7FFul, 0x8700FFFFul, 0x875F00FFul, 0x875F5FFFul,
      0x875F87FFul, 0x875FAFFFul, 0x875FD7FFul, 0x875FFFFFul, 0x878700FFul,
      0x87875FFFul, 0x878787FFul, 0x8787AFFFul, 0x8787D7FFul, 0x8787FFFFul,
      0x87AF00FFul, 0x87AF5FFFul, 0x87AF87FFul, 0x87AFAFFFul, 0x87AFD7FFul,
      0x87AFFFFFul, 0x87D700FFul, 0x87D75FFFul, 0x87D787FFul, 0x87D7AFFFul,
      0x87D7D7FFul, 0x87D7FFFFul, 0x87FF00FFul, 0x87FF5FFFul, 0x87FF87FFul,
      0x87FFAFFFul, 0x87FFD7FFul, 0x87FFFFFFul, 0xAF0000FFul, 0xAF005FFFul,
      0xAF0087FFul, 0xAF00AFFFul, 0xAF00D7FFul, 0xAF00FFFFul, 0xAF5F00FFul,
      0xAF5F5FFFul, 0xAF5F87FFul, 0xAF5FAFFFul, 0xAF5FD7FFul, 0xAF5FFFFFul,
      0xAF8700FFul, 0xAF875FFFul, 0xAF8787FFul, 0xAF87AFFFul, 0xAF87D7FFul,
      0xAF87FFFFul, 0xAFAF00FFul, 0xAFAF5FFFul, 0xAFAF87FFul, 0xAFAFAFFFul,
      0xAFAFD7FFul, 0xAFAFFFFFul, 0xAFD700FFul, 0xAFD75FFFul, 0xAFD787FFul,
      0xAFD7AFFFul, 0xAFD7D7FFul, 0xAFD7FFFFul, 0xAFFF00FFul, 0xAFFF5FFFul,
      0xAFFF87FFul, 0xAFFFAFFFul, 0xAFFFD7FFul, 0xAFFFFFFFul, 0xD70000FFul,
      0xD7005FFFul, 0xD70087FFul, 0xD700AFFFul, 0xD700D7FFul, 0xD700FFFFul,
      0xD75F00FFul, 0xD75F5FFFul, 0xD75F87FFul, 0xD75FAFFFul, 0xD75FD7FFul,
      0xD75FFFFFul, 0xD78700FFul, 0xD7875FFFul, 0xD78787FFul, 0xD787AFFFul,
      0xD787D7FFul, 0xD787FFFFul, 0xD7AF00FFul, 0xD7AF5FFFul, 0xD7AF87FFul,
      0xD7AFAFFFul, 0xD7AFD7FFul, 0xD7AFFFFFul, 0xD7D700FFul, 0xD7D75FFFul,
      0xD7D787FFul, 0xD7D7AFFFul, 0xD7D7D7FFul, 0xD7D7FFFFul, 0xD7FF00FFul,
      0xD7FF5FFFul, 0xD7FF87FFul, 0xD7FFAFFFul, 0xD7FFD7FFul, 0xD7FFFFFFul,
      0xFF0000FFul, 0xFF005FFFul, 0xFF0087FFul, 0xFF00AFFFul, 0xFF00D7FFul,
      0xFF00FFFFul, 0xFF5F00FFul, 0xFF5F5FFFul, 0xFF5F87FFul, 0xFF5FAFFFul,
      0xFF5FD7FFul, 0xFF5FFFFFul, 0xFF8700FFul, 0xFF875FFFul, 0xFF8787FFul,
      0xFF87AFFFul, 0xFF87D7FFul, 0xFF87FFFFul, 0xFFAF00FFul, 0xFFAF5FFFul,
      0xFFAF87FFul, 0xFFAFAFFFul, 0xFFAFD7FFul, 0xFFAFFFFFul, 0xFFD700FFul,
      0xFFD75FFFul, 0xFFD787FFul, 0xFFD7AFFFul, 0xFFD7D7FFul, 0xFFD7FFFFul,
      0xFFFF00FFul, 0xFFFF5FFFul, 0xFFFF87FFul, 0xFFFFAFFFul, 0xFFFFD7FFul,
      0xFFFFFFFFul,
      // 232 - 255: Grayscale Ramp
      0x080808FFul, 0x121212FFul, 0x1C1C1CFFul, 0x262626FFul, 0x303030FFul,
      0x3A3A3AFFul, 0x444444FFul, 0x4E4E4EFFul, 0x585858FFul, 0x626262FFul,
      0x6C6C6CFFul, 0x767676FFul, 0x808080FFul, 0x8A8A8AFFul, 0x949494FFul,
      0x9E9E9EFFul, 0xA8A8A8FFul, 0xB2B2B2FFul, 0xBCBCBCFFul, 0xC6C6C6FFul,
      0xD0D0D0FFul, 0xDADADAFFul, 0xE4E4E4FFul, 0xEEEEEEFFul};
  return xterm_palette[col];
}

static void vt100_execute_csi(char c, int *p, int n) {
  int a = (n > 0 && p[0]) ? p[0] : 1;
  switch (c) {
  case 'A':
    // mv up by a rows
    vt100_attempt_m(0, -(a * 16));
    break;
  case 'B':
    // mv down by a rows
    vt100_attempt_m(0, a * 16);
    break;
  case 'C':
    // mv forward by a cols
    vt100_attempt_m(a * 8, 0);
    break;
  case 'D':
    // mv backwards by a cols
    vt100_attempt_m(-(a * 8), 0);
    break;
  case 'H':
  case 'f': {
    // cursor position (1-based row;col)
    int row = (n > 0 && p[0]) ? p[0] : 1;
    int col = (n > 1 && p[1]) ? p[1] : 1;
    vt100_attempt_s((col - 1) * 8, (row - 1) * 16);
    break;
  }
  case 'J':
    // erase display
    fb0.clear(vt100_context.bgcol, &fb0);
    break;
  case 'K':
    // erase line
    fb0.fill_rect(0, vt100_context.y, fb0.width, 16, vt100_context.bgcol, &fb0);
    break;
  case 'm': {
    if (n < 1) {
      vt100_reset();
      break;
    }
    uint32_t tmp;
    for (int i = 0; i < n; i++) {
      switch (p[i]) {
      case 0:
        vt100_reset();
        break;
      case 1:
        // bold (nop)
        break;
      case 4:
        vt100_context.underline = true;
        break;
      case 7:
        if (!vt100_context.revideo) {
          tmp = vt100_context.col;
          vt100_context.col = vt100_context.bgcol;
          vt100_context.bgcol = tmp;
          vt100_context.revideo = true;
        }
        break;
      case 22:
        // unbold (nop)
        break;
      case 24:
        vt100_context.underline = false;
        break;
      case 27:
        if (vt100_context.revideo) {
          tmp = vt100_context.col;
          vt100_context.col = vt100_context.bgcol;
          vt100_context.bgcol = tmp;
          vt100_context.revideo = false;
        }
        break;
      // fg colors
      case 30:
        vt100_set_col(VT100_BLACK);
        break;
      case 31:
        vt100_set_col(VT100_RED);
        break;
      case 32:
        vt100_set_col(VT100_GREEN);
        break;
      case 33:
        vt100_set_col(VT100_YELLOW);
        break;
      case 34:
        vt100_set_col(VT100_BLUE);
        break;
      case 35:
        vt100_set_col(VT100_MAGENTA);
        break;
      case 36:
        vt100_set_col(VT100_CYAN);
        break;
      case 37:
        vt100_set_col(VT100_WHITE);
        break;
      case 38: {
        if (i + 1 >= n)
          goto invalidcmd;

        if (p[i + 1] == 2) {
          if (i + 4 >= n)
            goto invalidcmd;

          int r = p[i + 2];
          int g = p[i + 3];
          int b = p[i + 4];

          if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
            goto invalidcmd;

          uint32_t col = ((uint32_t)r << 24) | ((uint32_t)g << 16) |
                         ((uint32_t)b << 8) | 0xFF;

          vt100_set_col(col);
          i += 4;
        } else if (p[i + 1] == 5) {
          if (i + 2 >= n)
            goto invalidcmd;
	  int col = p[i + 2];
	  vt100_set_col(vt100_xterm_col(col));
          i += 2;
        } else {
          goto invalidcmd;
        }
        break;
      }
      case 39:
        if (vt100_context.revideo)
          vt100_context.bgcol = 0xAAAAAAFF;
        else
          vt100_context.col = 0xAAAAAAFF;
        break;
      // bg colours
      case 40:
        vt100_set_bgcol(VT100_BLACK);
        break;
      case 41:
        vt100_set_bgcol(VT100_RED);
        break;
      case 42:
        vt100_set_bgcol(VT100_GREEN);
        break;
      case 43:
        vt100_set_bgcol(VT100_YELLOW);
        break;
      case 44:
        vt100_set_bgcol(VT100_BLUE);
        break;
      case 45:
        vt100_set_bgcol(VT100_MAGENTA);
        break;
      case 46:
        vt100_set_bgcol(VT100_CYAN);
        break;
      case 47:
        vt100_set_bgcol(VT100_WHITE);
        break;
      case 48: {
        if (i + 1 >= n)
          goto invalidcmd;

        if (p[i + 1] == 2) {
          if (i + 4 >= n)
            goto invalidcmd;

          int r = p[i + 2];
          int g = p[i + 3];
          int b = p[i + 4];

          if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
            goto invalidcmd;

          uint32_t col = ((uint32_t)r << 24) | ((uint32_t)g << 16) |
                         ((uint32_t)b << 8) | 0xFF;

          vt100_set_bgcol(col);
          i += 4;
        } else if (p[i + 1] == 5) {
          if (i + 2 >= n)
            goto invalidcmd;
	  int col = p[i + 2];
	  vt100_set_bgcol(vt100_xterm_col(col));
          i += 2;
        } else {
          goto invalidcmd;
        }
        break;
      }

      case 49:
        // Default background
        if (vt100_context.revideo)
          vt100_context.col = 0x000000FF;
        else
          vt100_context.bgcol = 0x000000FF;
        break;
      // Bright Colors! OwO
      case 90:
        vt100_set_col(VT100_BRIGHT_BLACK);
        break;
      case 91:
        vt100_set_col(VT100_BRIGHT_RED);
        break;
      case 92:
        vt100_set_col(VT100_BRIGHT_GREEN);
        break;
      case 93:
        vt100_set_col(VT100_BRIGHT_YELLOW);
        break;
      case 94:
        vt100_set_col(VT100_BRIGHT_BLUE);
        break;
      case 95:
        vt100_set_col(VT100_BRIGHT_MAGENTA);
        break;
      case 96:
        vt100_set_col(VT100_BRIGHT_CYAN);
        break;
      case 97:
        vt100_set_col(VT100_BRIGHT_WHITE);
        break;

      case 100:
        vt100_set_bgcol(VT100_BRIGHT_BLACK);
        break;
      case 101:
        vt100_set_bgcol(VT100_BRIGHT_RED);
        break;
      case 102:
        vt100_set_bgcol(VT100_BRIGHT_GREEN);
        break;
      case 103:
        vt100_set_bgcol(VT100_BRIGHT_YELLOW);
        break;
      case 104:
        vt100_set_bgcol(VT100_BRIGHT_BLUE);
        break;
      case 105:
        vt100_set_bgcol(VT100_BRIGHT_MAGENTA);
        break;
      case 106:
        vt100_set_bgcol(VT100_BRIGHT_CYAN);
        break;
      case 107:
        vt100_set_bgcol(VT100_BRIGHT_WHITE);
        break;
      default:
        break;
      }
    }

    break;
  }
  default:
  invalidcmd:
    break;
  }
}

static void vt100_parse_csi(char *str) {
  int params[VT100_MAX_PARAMS] = {0};
  int nparams = 0;
  int v = 0;
  int have_v = 0;

  while (*str) {
    char c = *(str++);
    if (c >= '0' && c <= '9') {
      v = v * 10 + (c - '0');
      have_v = 1;
      continue;
    }

    if (c == ';') {
      if (nparams < VT100_MAX_PARAMS)
        params[nparams++] = have_v ? v : 0;

      v = 0;
      have_v = 0;
      continue;
    }

    // CSI final byte: 0x40–0x7E
    if (c >= 0x40 && c <= 0x7E) {
      if (nparams < VT100_MAX_PARAMS)
        params[nparams++] = have_v ? v : 0;

      vt100_execute_csi(c, params, nparams);
      return;
    }

    // Unsupported or malformed sequence.
    return;
  }
}

void vt100_write_stream(char *str) {
  for (int i = 0; str[i] != '\0'; ++i) {
    if (str[i] == '\x1b') {
      // escape
      if (str[++i] == '[') {
        // has CSI; find the final byte so the params aren't printed literally
        int end = i + 1;
        while (str[end] != '\0' && !(str[end] >= 0x40 && str[end] <= 0x7E))
          ++end;
        // has CSI; do escapy stuff
        vt100_parse_csi(str + i + 1);
        i = (str[end] != '\0') ? end : end - 1;
      }
      continue;
    }
    if (str[i] == '\n') {
      vt100_newline();
      continue;
    } else if (str[i] == '\r') {
      vt100_context.x = 0;
      continue;
    } else if (str[i] == '\t') {
      vt100_context.x += 32;
      if (vt100_context.x + 8 > (int32_t)fb0.width) {
        vt100_newline();
      }
      continue;
    } else if (str[i] == '\b') {
      if (vt100_context.x > 8)
        vt100_context.x -= 8;
      continue;
    } else if (str[i] < 0x20 || str[i] > 0x7E) {
      vt100_write_char('X');
    } else {
      vt100_write_char(str[i]);
    }
    vt100_context.x += 8;
    if (vt100_context.x + 8 > (int32_t)fb0.width) {
      vt100_newline();
    }
  }
  fb0.update_fb(&fb0);
}
