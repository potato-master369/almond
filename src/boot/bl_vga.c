#include "bl_vga.h"
#include "bl_ports.h"
#include "drv/terminus.h"

#define VGA_OFFSET 0xB8000

struct {
  uint8_t attr;
  uint16_t cur_x;
  uint16_t cur_y;
} bl_vga_data;

static const uint32_t bl_vga_palette[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA,
    0xAA5500, 0xAAAAAA, 0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF};

static uint32_t bl_vga_component(uint8_t value, uint8_t size,
                                 uint8_t position) {
  if (size >= 8) {
    return (uint32_t)value << position;
  }
  return (uint32_t)(value >> (8 - size)) << position;
}

static uint32_t bl_vga_color(uint8_t index) {
  uint32_t rgb = bl_vga_palette[index & 0x0F];
  return bl_vga_component((uint8_t)(rgb >> 16), bl_vbe_red_mask,
                          bl_vbe_red_position) |
         bl_vga_component((uint8_t)(rgb >> 8), bl_vbe_green_mask,
                          bl_vbe_green_position) |
         bl_vga_component((uint8_t)rgb, bl_vbe_blue_mask, bl_vbe_blue_position);
}

static void bl_vga_put_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x < bl_vbe_width && y < bl_vbe_height) {
    uint8_t bytes_per_pixel = (bl_vbe_bpp + 7) / 8;
    volatile uint8_t *pixel =
        (volatile uint8_t *)(bl_vbe_framebuffer + y * bl_vbe_pitch +
                             x * bytes_per_pixel);
    for (uint8_t byte = 0; byte < bytes_per_pixel; ++byte) {
      pixel[byte] = (uint8_t)(color >> (byte * 8));
    }
  }
}

static void bl_vga_draw_char(char character, uint16_t cell_x, uint16_t cell_y,
                             uint8_t attr) {
  uint8_t glyph = (uint8_t)character;
  uint32_t foreground = bl_vga_color(attr & 0x0F);
  uint32_t background = bl_vga_color(attr >> 4);
  if (glyph >= 128) {
    glyph = '?';
  }
  for (uint16_t y = 0; y < 16; ++y) {
    uint8_t row = terminus_fon[glyph][y];
    for (uint16_t x = 0; x < 8; ++x) {
      uint8_t bit_mask = (1 << x);

      bl_vga_put_pixel(cell_x * 8 + x, cell_y * 16 + y,
                       (row & bit_mask) ? foreground : background);
    }
  }
}

static void bl_vga_clear_row(uint16_t row) {
  uint32_t background = bl_vga_color(bl_vga_data.attr >> 4);
  for (uint16_t y = 0; y < 16; ++y) {
    for (uint16_t x = 0; x < bl_vbe_width; ++x) {
      bl_vga_put_pixel(x, row * 16 + y, background);
    }
  }
}

void bl_vga_write_char(char a, uint16_t cur_x, uint16_t cur_y,
                       unsigned char attr) {
  if (bl_vbe_framebuffer != 0) {
    bl_vga_draw_char(a, cur_x, cur_y, attr);
    return;
  }
  uint16_t pos = (cur_y * 80 + cur_x);
  volatile uint16_t *where = (volatile uint16_t *)VGA_OFFSET + pos;
  *where = (uint16_t)a | (attr << 8);
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void bl_vga_init(void) {
  bl_vga_data.attr = 0x0F;
  bl_vga_data.cur_x = 0;
  bl_vga_data.cur_y = 0;
  if (bl_vbe_framebuffer != 0) {
    for (uint16_t row = 0; row < bl_vbe_height / 16; ++row) {
      bl_vga_clear_row(row);
    }
  } else {
    for (int x = 0; x < (bl_vbe_width / 8); ++x) {
      for (int y = 0; y < (bl_vbe_height / 16); ++y) {
        bl_vga_write_char(' ', x, y, 0x0F);
      }
    }
  }
}

void bl_vga_set_x(uint8_t cur_x) { bl_vga_data.cur_x = cur_x; }
void bl_vga_set_y(uint8_t cur_y) { bl_vga_data.cur_y = cur_y; }
void bl_vga_set_attr(uint8_t attr) { bl_vga_data.attr = attr; }

void bl_vga_scroll_up(void) {
  if (bl_vbe_framebuffer != 0) {
    volatile uint8_t *framebuffer = (volatile uint8_t *)bl_vbe_framebuffer;
    uint32_t bytes_per_line = bl_vbe_width * ((bl_vbe_bpp + 7) / 8);
    uint32_t rows = bl_vbe_height / 16;
    for (uint32_t y = 0; y + 16 < rows * 16; ++y) {
      for (uint32_t x = 0; x < bytes_per_line; ++x) {
        framebuffer[y * bl_vbe_pitch + x] =
            framebuffer[(y + 16) * bl_vbe_pitch + x];
      }
    }
    bl_vga_clear_row((uint16_t)(rows - 1));
    return;
  }

  uint16_t *vga = (uint16_t *)VGA_OFFSET;
  for (int i = 0; i < 24 * 80; i++) {
    vga[i] = vga[i + 80];
  }

  uint16_t blank = (0x07 << 8) | ' ';
  for (int i = 24 * 80; i < 25 * 80; i++) {
    vga[i] = blank;
  }
}

void bl_vga_write(const char *s, unsigned char attr) {
  bl_vga_data.attr = attr;
  uint16_t columns = bl_vbe_framebuffer != 0 ? bl_vbe_width / 8 : 80;
  uint16_t rows = bl_vbe_framebuffer != 0 ? bl_vbe_height / 16 : 25;
  int i = 0;
  while (s[i] != '\0') {
    if (s[i] == '\n') {
      bl_vga_data.cur_x = 0;
      ++bl_vga_data.cur_y;
    } else if (s[i] == '\t') {
      for (int j = 0; j < 4; ++j) {
        if (bl_vga_data.cur_x >= columns) {
          bl_vga_data.cur_x = 0;
          ++bl_vga_data.cur_y;
        }
        // Handle vertical scroll if wrapping pushed us past the bottom
        if (bl_vga_data.cur_y >= rows) {
          bl_vga_scroll_up();
          bl_vga_data.cur_y = rows - 1;
        }
        bl_vga_write_char(' ', bl_vga_data.cur_x, bl_vga_data.cur_y,
                          bl_vga_data.attr);
        ++bl_vga_data.cur_x;
      }
    } else if (s[i] >= 0x20 && s[i] <= 0x7E) {
      if (bl_vga_data.cur_x >= columns) {
        bl_vga_data.cur_x = 0;
        ++bl_vga_data.cur_y;
      }
      if (bl_vga_data.cur_y >= rows) {
        bl_vga_scroll_up();
        bl_vga_data.cur_y = rows - 1;
      }
      bl_vga_write_char(s[i], bl_vga_data.cur_x, bl_vga_data.cur_y,
                        bl_vga_data.attr);
      ++bl_vga_data.cur_x;
    }

    // Final safety check for row overflow on explicit \n
    if (bl_vga_data.cur_y >= rows) {
      bl_vga_scroll_up();
      bl_vga_data.cur_y = rows - 1;
    }
    ++i;
  }
}

void bl_vga_write_1(const char a, unsigned char attr) {
  uint16_t columns = bl_vbe_framebuffer != 0 ? bl_vbe_width / 8 : 80;
  bl_vga_write_char(a, bl_vga_data.cur_x, bl_vga_data.cur_y, attr);
  ++bl_vga_data.cur_x;
  if (bl_vga_data.cur_x >= columns) {
    bl_vga_scroll_up();
    bl_vga_data.cur_x = 0;
  }
}

void bl_vga_backspace(unsigned char attr) {
  if (bl_vga_data.cur_x > 0) {
    --bl_vga_data.cur_x;
  }
  bl_vga_write_char(' ', bl_vga_data.cur_x, bl_vga_data.cur_y, attr);
}
