// panic.c
// ---------------------------
// Kernel Panic screen. Depends on VBE buffer
#include "framebuffer/fb.h"
#include "helpers/string.h"
#include "kernel_types.h"

extern fb_surface_t fb0;
extern const uint8_t terminus_fon[128][16];
static uint16_t panic_x = 0;
static uint16_t panic_y = 0;

static void panic_print_sub(const char *msg) {
    int i = 0;
    while (msg[i] != '\0') {
        if (msg[i] == '\n') {
            panic_x = 0;
            panic_y += 16;
            i++;
            continue;
        }

        for (uint16_t y = 0; y < 16; ++y) {
            uint8_t row = terminus_fon[(unsigned char)msg[i]][y];
            for (uint16_t x = 0; x < 8; ++x) {
                fb0.write_pixel(panic_x + x, panic_y + y, (row & (1 << x)) ? 0xffffffff : 0x001e7bff, &fb0);
            }
        }

        panic_x += 8;
	if (panic_x >= fb0.width - 8) {
          panic_x = 0;
	  panic_y += 16;
	}
        ++i;
    }
}

void panic_print(uint32_t err, const char *pretty_name, const char *file, int line) {
  char reportbuf[256];
  for (uint32_t x = 0; x < fb0.width; ++x) {
    for (uint32_t y = 0; y < fb0.height; ++y) {
      fb0.write_pixel(x, y, 0x001e7b00, &fb0);
    }
  }
  panic_print_sub("Almond kernel\n\nCompile time: " __DATE__ " " __TIME__ "\n\n");
  itoa_hex(err, reportbuf);
  panic_print_sub("  STOP: 0x");
  panic_print_sub(reportbuf);
  panic_print_sub(" (");
  panic_print_sub(pretty_name);
  panic_print_sub(")\n\n");
  switch(err) {
    case 0xD001:
	    panic_print_sub("    The kernel's contract had an invalid magic number. Are you using the ABL (Almond BootLoader)? Almond cannot be booted from other bootloaders. If you are, check that your bootloader/kernel is not corrupted.\n\n");
	    break;
    default:
	    panic_print_sub("    No further information is available. Consult the manual or check the source in the below files for more help. Ensure your computer is functioning properly and that your hard disk is not failing.\n\n");
	    break;
  }
  panic_print_sub(" Fault at: ");
  panic_print_sub(file);
  itoa(line, reportbuf);
  panic_print_sub(":");
  panic_print_sub(reportbuf);

  fb0.update_fb(&fb0);
  for (;;)
	  ;
}


#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

static void vga_print_char(char c, int row, int col, uint8_t color) {
    uint16_t* vga_buffer = (uint16_t*) VGA_ADDRESS;
    const int index = row * VGA_WIDTH + col;
    
    vga_buffer[index] = ((uint16_t) color << 8) | (uint16_t) c;
}

static int current_col = 0;
static int current_row = 0;

static void vga_print_string(const char* str, uint8_t color) {

    while (*str != 0) {
        if (*str == '\n') {
            current_row++;
            current_col = 0;
        } else {
            vga_print_char(*str, current_row, current_col, color);
            current_col++;

            if (current_col >= VGA_WIDTH) {
                current_col = 0;
                current_row++;
            }
        }
        str++;
        
        // Stop if we exceed the screen height
        if (current_row >= VGA_HEIGHT) {
            break;
        }
    }
}

void panic_vga_panic(void) {
  for (int x = 0; x < VGA_WIDTH; ++x) {
    for (int y = 0; y < VGA_HEIGHT; ++y) {
      vga_print_char(' ', y, x, 0x1F);
    }
  }
  vga_print_string("Almond kernel\n\nCompile time: " __DATE__ " " __TIME__ "\n\nSTOP 0xDFFFF (VGA_NOT_FOUND)\n\n", 0x1F);
  vga_print_string("    No suitable graphics adapter was found. This text is printing on text mode. Check your video card is supported.\n", 0x1F);

  for (;;)
	  ;
}