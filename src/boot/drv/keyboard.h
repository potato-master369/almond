#ifndef BL_KEYBOARD_H
#define BL_KEYBOARD_H
unsigned char keyboard_read_scancode(void);
void keyboard_set_keymap(unsigned char keymap);
char keyboard_get_ascii(unsigned char scancode);
char keyboard_mod_state(unsigned char scancode);
void keyboard_reboot(void);
#endif
