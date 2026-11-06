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
  uint32_t x;
  uint32_t y;
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
}

static void vt100_write_char(char c) {
  for (uint16_t y = 0; y < 16; ++y) {
    uint8_t row = terminus_fon[c][y];
    for (uint16_t x = 0; x < 8; ++x) {
      fb0.write_pixel(
          vt100_context.x + x, vt100_context.y + y,
          (row & (1 << x)) ? vt100_context.col : vt100_context.bgcol, &fb0);
    }
  }
}

static inline void vt100_newline(void) {
  vt100_context.x = 0;

  if (vt100_context.y + 16 >= fb0.height) {
    fb0.scroll_up(16, vt100_context.bgcol, &fb0);
    vt100_context.y = (fb0.height >= 16) ? fb0.height - 16 : 0;
  } else {
    vt100_context.y += 16;
  }
}

static inline void vt100_attempt_m(int x, int y) {
  vt100_context.x += x;
  if (vt100_context.x < 0)
    vt100_context.x = 0;
  else if (vt100_context.x + 8 > fb0.width)
    vt100_context.x = fb0.width - 8;
  vt100_context.y += y;
  if (vt100_context.y < 0)
    vt100_context.y = 0;
  else if (vt100_context.y + 16 > fb0.height)
    vt100_context.y = fb0.height - 8;
}
static inline void vt100_attempt_s(int x, int y) {
  vt100_context.x = x;
  if (vt100_context.x < 0)
    vt100_context.x = 0;
  else if (vt100_context.x + 8 > fb0.width)
    vt100_context.x = fb0.width - 8;
  vt100_context.y = y;
  if (vt100_context.y < 0)
    vt100_context.y = 0;
  else if (vt100_context.y + 16 > fb0.height)
    vt100_context.y = fb0.height - 8;
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

static void vt100_execute_csi(char c, int *p, int n) {
  int a = (n > 0 && p[0]) ? p[0] : 1;
  switch (c) {
  case 'A':
    // mv up by a rows
    vt100_attempt_m(0, -(a * 8));
    break;
  case 'B':
    // mv down by a rows
    vt100_attempt_m(0, a * 8);
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
  case 'f':
    // cursor position
    if (n == 2) {
      vt100_attempt_s(p[0], p[1]);
    } else
      goto invalidcmd;
    break;
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
        case 30: vt100_set_col(VT100_BLACK); break;
        case 31: vt100_set_col(VT100_RED); break;
        case 32: vt100_set_col(VT100_GREEN); break;
        case 33: vt100_set_col(VT100_YELLOW); break;
        case 34: vt100_set_col(VT100_BLUE); break;
        case 35: vt100_set_col(VT100_MAGENTA); break;
        case 36: vt100_set_col(VT100_CYAN); break;
        case 37: vt100_set_col(VT100_WHITE); break;
        case 38: {
            if (i + 1 >= n)
                goto invalidcmd;

            if (p[i + 1] == 2) {
                if (i + 4 >= n)
                    goto invalidcmd;

                int r = p[i + 2];
                int g = p[i + 3];
                int b = p[i + 4];

                if (r < 0 || r > 255 ||
                    g < 0 || g > 255 ||
                    b < 0 || b > 255)
                    goto invalidcmd;

                uint32_t col =
                    ((uint32_t)r << 24) |
                    ((uint32_t)g << 16) |
                    ((uint32_t)b << 8) |
                    0xFF;

                vt100_set_col(col);
                i += 4;
            } else if (p[i + 1] == 5) {
                // FIXME: xterm-256col colours
                if (i + 2 >= n)
                    goto invalidcmd;
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
        case 40: vt100_set_bgcol(VT100_BLACK); break;
        case 41: vt100_set_bgcol(VT100_RED); break;
        case 42: vt100_set_bgcol(VT100_GREEN); break;
        case 43: vt100_set_bgcol(VT100_YELLOW); break;
        case 44: vt100_set_bgcol(VT100_BLUE); break;
        case 45: vt100_set_bgcol(VT100_MAGENTA); break;
        case 46: vt100_set_bgcol(VT100_CYAN); break;
        case 47: vt100_set_bgcol(VT100_WHITE); break;
        case 48: {
            if (i + 1 >= n)
                goto invalidcmd;

            if (p[i + 1] == 2) {
                if (i + 4 >= n)
                    goto invalidcmd;

                int r = p[i + 2];
                int g = p[i + 3];
                int b = p[i + 4];

                if (r < 0 || r > 255 ||
                    g < 0 || g > 255 ||
                    b < 0 || b > 255)
                    goto invalidcmd;

                uint32_t col =
                    ((uint32_t)r << 24) |
                    ((uint32_t)g << 16) |
                    ((uint32_t)b << 8) |
                    0xFF;

                vt100_set_bgcol(col);
                i += 4;
            } else if (p[i + 1] == 5) {
                // FIXME: xterm-256col colours
                if (i + 2 >= n)
                    goto invalidcmd;
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
        case 90: vt100_set_col(VT100_BRIGHT_BLACK); break;
        case 91: vt100_set_col(VT100_BRIGHT_RED); break;
        case 92: vt100_set_col(VT100_BRIGHT_GREEN); break;
        case 93: vt100_set_col(VT100_BRIGHT_YELLOW); break;
        case 94: vt100_set_col(VT100_BRIGHT_BLUE); break;
        case 95: vt100_set_col(VT100_BRIGHT_MAGENTA); break;
        case 96: vt100_set_col(VT100_BRIGHT_CYAN); break;
        case 97: vt100_set_col(VT100_BRIGHT_WHITE); break;

        case 100: vt100_set_bgcol(VT100_BRIGHT_BLACK); break;
        case 101: vt100_set_bgcol(VT100_BRIGHT_RED); break;
        case 102: vt100_set_bgcol(VT100_BRIGHT_GREEN); break;
        case 103: vt100_set_bgcol(VT100_BRIGHT_YELLOW); break;
        case 104: vt100_set_bgcol(VT100_BRIGHT_BLUE); break;
        case 105: vt100_set_bgcol(VT100_BRIGHT_MAGENTA); break;
        case 106: vt100_set_bgcol(VT100_BRIGHT_CYAN); break;
        case 107: vt100_set_bgcol(VT100_BRIGHT_WHITE); break;
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
        // has CSI; do escapy stuff
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
      if (vt100_context.x + 8 > fb0.width) {
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
    if (vt100_context.x + 8 > fb0.width) {
      vt100_newline();
    }
  }
}
