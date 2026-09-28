// keyboard.c
// ----------------------------------
// p/s2 keyboard driver for ABL
// MIT license; Copyright (C) potato-master369 2026-
#include "../bl_ports.h"

#define PS2_PORT_STATUS 0x64
#define PS2_PORT_DATA 0x60
#define PS2_STATUS_OBF 0x01

// returns a scancode
unsigned char keyboard_read_scancode(void) {
  while (!(inb(PS2_PORT_STATUS) & PS2_STATUS_OBF)) {
    // do nothing
    __asm__ volatile("pause");
  }
  return inb(PS2_PORT_DATA);
}

bool_t shift_held = 0;
bool_t caps_on = 0;
// keycode to ASCII conversion
#define SCANCODE_MAX 0x85
#define LAYOUT_US 0
#define LAYOUT_JIS 1
#define LAYOUT_ISO 2
unsigned char layout = LAYOUT_US;
const char scancode_to_ascii[SCANCODE_MAX][3][2] = {
    // Letters (Identical across US, JIS, ISO)
    [0x1E] = { {'a', 'A'}, {'a', 'A'}, {'a', 'A'} },
    [0x30] = { {'b', 'B'}, {'b', 'B'}, {'b', 'B'} },
    [0x2E] = { {'c', 'C'}, {'c', 'C'}, {'c', 'C'} },
    [0x20] = { {'d', 'D'}, {'d', 'D'}, {'d', 'D'} },
    [0x12] = { {'e', 'E'}, {'e', 'E'}, {'e', 'E'} },
    [0x21] = { {'f', 'F'}, {'f', 'F'}, {'f', 'F'} },
    [0x22] = { {'g', 'G'}, {'g', 'G'}, {'g', 'G'} },
    [0x23] = { {'h', 'H'}, {'h', 'H'}, {'h', 'H'} },
    [0x17] = { {'i', 'I'}, {'i', 'I'}, {'i', 'I'} },
    [0x24] = { {'j', 'J'}, {'j', 'J'}, {'j', 'J'} },
    [0x25] = { {'k', 'K'}, {'k', 'K'}, {'k', 'K'} },
    [0x26] = { {'l', 'L'}, {'l', 'L'}, {'l', 'L'} },
    [0x32] = { {'m', 'M'}, {'m', 'M'}, {'m', 'M'} },
    [0x31] = { {'n', 'N'}, {'n', 'N'}, {'n', 'N'} },
    [0x18] = { {'o', 'O'}, {'o', 'O'}, {'o', 'O'} },
    [0x19] = { {'p', 'P'}, {'p', 'P'}, {'p', 'P'} },
    [0x10] = { {'q', 'Q'}, {'q', 'Q'}, {'q', 'Q'} },
    [0x13] = { {'r', 'R'}, {'r', 'R'}, {'r', 'R'} },
    [0x1F] = { {'s', 'S'}, {'s', 'S'}, {'s', 'S'} },
    [0x14] = { {'t', 'T'}, {'t', 'T'}, {'t', 'T'} },
    [0x16] = { {'u', 'U'}, {'u', 'U'}, {'u', 'U'} },
    [0x2F] = { {'v', 'V'}, {'v', 'V'}, {'v', 'V'} },
    [0x11] = { {'w', 'W'}, {'w', 'W'}, {'w', 'W'} },
    [0x2D] = { {'x', 'X'}, {'x', 'X'}, {'x', 'X'} },
    [0x15] = { {'y', 'Y'}, {'y', 'Y'}, {'y', 'Y'} },
    [0x2C] = { {'z', 'Z'}, {'z', 'Z'}, {'z', 'Z'} },

    // Core Numbers (Varying shifted states)
    [0x02] = { {'1', '!'}, {'1', '!'}, {'1', '!'} },
    [0x03] = { {'2', '@'}, {'2', '"'}, {'2', '"'} }, // US: @ | JIS/ISO: "
    [0x04] = { {'3', '#'}, {'3', '#'}, {'3', 'x'} },   // ISO uses £ symbol
    [0x05] = { {'4', '$'}, {'4', '$'}, {'4', '$'} },
    [0x06] = { {'5', '%'}, {'5', '%'}, {'5', '%'} },
    [0x07] = { {'6', '^'}, {'6', '&'}, {'6', '^'} }, // JIS shifted 6 is &
    [0x08] = { {'7', '&'}, {'7', '\''},{'7', '/'} }, // JIS: ' | ISO: /
    [0x09] = { {'8', '*'}, {'8', '('}, {'8', '('} }, // JIS/ISO shifted 8 is (
    [0x0A] = { {'9', '('}, {'9', ')'}, {'9', ')'} }, // JIS/ISO shifted 9 is )
    [0x0B] = { {'0', ')'}, {'0', ' '}, {'0', '='} }, // JIS: Unshifted 0 is 0, Shifted is blank/mod | ISO: =

    // White Spaces / Control Keys
    [0x39] = { {' ', ' '}, {' ', ' '}, {' ', ' '} },
    [0x1C] = { {'\n', '\n'}, {'\n', '\n'}, {'\n', '\n'} },
    [0x0E] = { {'\b', '\b'}, {'\b', '\b'}, {'\b', '\b'} },
    [0x0F] = { {'\t', '\t'}, {'\t', '\t'}, {'\t', '\t'} },

    // Symbols & Region Specific Variations
    [0x29] = { {'`', '~'}, {'^', '~'}, {'`', 'x'} }, // US: ` | JIS: ^ | ISO: `
    [0x0C] = { {'-', '_'}, {'-', '='}, {'-', '_'} }, // JIS shifted - is =
    [0x0D] = { {'=', '+'}, {'x', '`'},  {'=', '+'} }, // JIS equals key
    [0x1A] = { {'[', '{'}, {'@', '`'},  {'[', '{'} }, // JIS [ key
    [0x1B] = { {']', '}'}, {'[', '{'},  {']', '}'} }, // JIS ] key
    [0x2B] = { {'\\', '|'},{0, 0},      {0, 0} },     // US ANSI Backslash
    [0x27] = { {';', ':'}, {';', '+'}, {';', ':'} }, // JIS shifted ; is +
    [0x28] = { {'\'', '"'},{':', '*'}, {'\'', '@'} },// US: ' | JIS: : | ISO: ' and @
    [0x33] = { {',', '<'}, {',', '<'}, {',', '<'} },
    [0x34] = { {'.', '>'}, {'.', '>'}, {'.', '>'} },
    [0x35] = { {'/', '?'}, {'/', '?'}, {'/', '?'} },

    // Special Physical Variant Keys (JIS/ISO Layout Extensions)
    [0x56] = { {0, 0},     {0, 0},     {'\\', '|'} },// ISO physical Extra Key
    [0x73] = { {0, 0},     {'\\', '_'},{0, 0} },     // JIS Ro key
    [0x7D] = { {0, 0},     {'\\', '|'},{0, 0} }      // JIS Yen key
};

char keyboard_get_ascii(unsigned char scancode) {
    // Ensure scancode is within bounds of the table
    if (scancode >= SCANCODE_MAX) {
        return 0;
    }

    // Ensure layout and shift flags are valid indices
    if (layout > 2) {
        return 0;
    }

    // Access the table: [scancode][layout][shifted]
    return scancode_to_ascii[scancode][layout][caps_on ? !shift_held : shift_held];
}

void keyboard_mod_state(unsigned char scancode) {
  switch (scancode) {
        case 0x2A: // left shift press
        case 0x36: // right shift press
            shift_held = 1;
            break;
        case 0xAA: // left shift release
        case 0xB6: // right shift release
            shift_held = 0;
            break;
        case 0x3A: // caps lock press
            caps_on = !caps_on;
            break;
        default:
            break;
    }
}
void keyboard_set_keymap(unsigned char keymap) {
  layout = keymap;
}

void keyboard_reboot(void) {
  // clear buffer
  unsigned char good = 0x02;
  while (good & 0x02) {
    good = inb(PS2_PORT_STATUS);
  }

  outb(PS2_PORT_STATUS, 0xFE); // command to reset
  for (;;)
	  __asm__("hlt");
}
