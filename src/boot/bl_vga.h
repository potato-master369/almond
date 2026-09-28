#ifndef BL_VGA_H
#define BL_VGA_H
#include "bl_types.h"
extern uint32_t bl_vbe_framebuffer;
extern uint16_t bl_vbe_pitch;
extern uint16_t bl_vbe_width;
extern uint16_t bl_vbe_height;
extern uint8_t bl_vbe_bpp;
extern uint8_t bl_vbe_red_mask;
extern uint8_t bl_vbe_red_position;
extern uint8_t bl_vbe_green_mask;
extern uint8_t bl_vbe_green_position;
extern uint8_t bl_vbe_blue_mask;
extern uint8_t bl_vbe_blue_position;
void bl_vga_write_char(char a, uint16_t cur_x, uint16_t cur_y, unsigned char attr);
void bl_vga_init(void);
void bl_vga_set_x(uint8_t cur_x);
void bl_vga_set_y(uint8_t cur_y);
void bl_vga_set_attr(uint8_t attr);
void bl_vga_write(const char *s, unsigned char attr);
void bl_vga_write_1(const char a, unsigned char attr);
void bl_vga_backspace(unsigned char attr);
#endif
