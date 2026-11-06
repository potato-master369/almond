// us.c
// ---------------------------
// en_US (US Layout) Keymap
#include "keymap.h"

almond_keymap_t fallback_keymap = {
    .magic = {'a', 'k', 'm', 'p'},
    .len = 216,
    .data = {
        // 'a' (0x04)
        {.key = 0x0004, .value = 'a'}, {.key = 0x0104, .value = 'A'}, {.key = 0x0204, .value = 'A'}, {.key = 0x0304, .value = 'a'},
        // 'b' (0x05)
        {.key = 0x0005, .value = 'b'}, {.key = 0x0105, .value = 'B'}, {.key = 0x0205, .value = 'B'}, {.key = 0x0305, .value = 'b'},
        // 'c' (0x06)
        {.key = 0x0006, .value = 'c'}, {.key = 0x0106, .value = 'C'}, {.key = 0x0206, .value = 'C'}, {.key = 0x0306, .value = 'c'},
        // 'd' (0x07)
        {.key = 0x0007, .value = 'd'}, {.key = 0x0107, .value = 'D'}, {.key = 0x0207, .value = 'D'}, {.key = 0x0307, .value = 'd'},
        // 'e' (0x08)
        {.key = 0x0008, .value = 'e'}, {.key = 0x0108, .value = 'E'}, {.key = 0x0208, .value = 'E'}, {.key = 0x0308, .value = 'e'},
        // 'f' (0x09)
        {.key = 0x0009, .value = 'f'}, {.key = 0x0109, .value = 'F'}, {.key = 0x0209, .value = 'F'}, {.key = 0x0309, .value = 'f'},
        // 'g' (0x0A)
        {.key = 0x000A, .value = 'g'}, {.key = 0x010A, .value = 'G'}, {.key = 0x020A, .value = 'G'}, {.key = 0x030A, .value = 'g'},
        // 'h' (0x0B)
        {.key = 0x000B, .value = 'h'}, {.key = 0x010B, .value = 'H'}, {.key = 0x020B, .value = 'H'}, {.key = 0x030B, .value = 'h'},
        // 'i' (0x0C)
        {.key = 0x000C, .value = 'i'}, {.key = 0x010C, .value = 'I'}, {.key = 0x020C, .value = 'I'}, {.key = 0x030C, .value = 'i'},
        // 'j' (0x0D)
        {.key = 0x000D, .value = 'j'}, {.key = 0x010D, .value = 'J'}, {.key = 0x020D, .value = 'J'}, {.key = 0x030D, .value = 'j'},
        // 'k' (0x0E)
        {.key = 0x000E, .value = 'k'}, {.key = 0x010E, .value = 'K'}, {.key = 0x020E, .value = 'K'}, {.key = 0x030E, .value = 'k'},
        // 'l' (0x0F)
        {.key = 0x000F, .value = 'l'}, {.key = 0x010F, .value = 'L'}, {.key = 0x020F, .value = 'L'}, {.key = 0x030F, .value = 'l'},
        // 'm' (0x10)
        {.key = 0x0010, .value = 'm'}, {.key = 0x0110, .value = 'M'}, {.key = 0x0210, .value = 'M'}, {.key = 0x0310, .value = 'm'},
        // 'n' (0x11)
        {.key = 0x0011, .value = 'n'}, {.key = 0x0111, .value = 'N'}, {.key = 0x0211, .value = 'N'}, {.key = 0x0311, .value = 'n'},
        // 'o' (0x12)
        {.key = 0x0012, .value = 'o'}, {.key = 0x0112, .value = 'O'}, {.key = 0x0212, .value = 'O'}, {.key = 0x0312, .value = 'o'},
        // 'p' (0x13)
        {.key = 0x0013, .value = 'p'}, {.key = 0x0113, .value = 'P'}, {.key = 0x0213, .value = 'P'}, {.key = 0x0313, .value = 'p'},
        // 'q' (0x14)
        {.key = 0x0014, .value = 'q'}, {.key = 0x0114, .value = 'Q'}, {.key = 0x0214, .value = 'Q'}, {.key = 0x0314, .value = 'q'},
        // 'r' (0x15)
        {.key = 0x0015, .value = 'r'}, {.key = 0x0115, .value = 'R'}, {.key = 0x0215, .value = 'R'}, {.key = 0x0315, .value = 'r'},
        // 's' (0x16)
        {.key = 0x0016, .value = 's'}, {.key = 0x0116, .value = 'S'}, {.key = 0x0216, .value = 'S'}, {.key = 0x0316, .value = 's'},
        // 't' (0x17)
        {.key = 0x0017, .value = 't'}, {.key = 0x0117, .value = 'T'}, {.key = 0x0217, .value = 'T'}, {.key = 0x0317, .value = 't'},
        // 'u' (0x18)
        {.key = 0x0018, .value = 'u'}, {.key = 0x0118, .value = 'U'}, {.key = 0x0218, .value = 'U'}, {.key = 0x0318, .value = 'u'},
        // 'v' (0x19)
        {.key = 0x0019, .value = 'v'}, {.key = 0x0119, .value = 'V'}, {.key = 0x0219, .value = 'V'}, {.key = 0x0319, .value = 'v'},
        // 'w' (0x1A)
        {.key = 0x001A, .value = 'w'}, {.key = 0x011A, .value = 'W'}, {.key = 0x021A, .value = 'W'}, {.key = 0x031A, .value = 'w'},
        // 'x' (0x1B)
        {.key = 0x001B, .value = 'x'}, {.key = 0x011B, .value = 'X'}, {.key = 0x021B, .value = 'X'}, {.key = 0x031B, .value = 'x'},
        // 'y' (0x1C)
        {.key = 0x001C, .value = 'y'}, {.key = 0x011C, .value = 'Y'}, {.key = 0x021C, .value = 'Y'}, {.key = 0x031C, .value = 'y'},
        // 'z' (0x1D)
        {.key = 0x001D, .value = 'z'}, {.key = 0x011D, .value = 'Z'}, {.key = 0x021D, .value = 'Z'}, {.key = 0x031D, .value = 'z'},

        // '1' (0x1E)
        {.key = 0x001E, .value = '1'}, {.key = 0x011E, .value = '1'}, {.key = 0x021E, .value = '!'}, {.key = 0x031E, .value = '!'},
        // '2' (0x1F)
        {.key = 0x001F, .value = '2'}, {.key = 0x011F, .value = '2'}, {.key = 0x021F, .value = '@'}, {.key = 0x031F, .value = '@'},
        // '3' (0x20)
        {.key = 0x0020, .value = '3'}, {.key = 0x0120, .value = '3'}, {.key = 0x0220, .value = '#'}, {.key = 0x0320, .value = '#'},
        // '4' (0x21)
        {.key = 0x0021, .value = '4'}, {.key = 0x0121, .value = '4'}, {.key = 0x0221, .value = '$'}, {.key = 0x0321, .value = '$'},
        // '5' (0x22)
        {.key = 0x0022, .value = '5'}, {.key = 0x0122, .value = '5'}, {.key = 0x0222, .value = '%'}, {.key = 0x0322, .value = '%'},
        // '6' (0x23)
        {.key = 0x0023, .value = '6'}, {.key = 0x0123, .value = '6'}, {.key = 0x0223, .value = '^'}, {.key = 0x0323, .value = '^'},
        // '7' (0x24)
        {.key = 0x0024, .value = '7'}, {.key = 0x0124, .value = '7'}, {.key = 0x0224, .value = '&'}, {.key = 0x0324, .value = '&'},
        // '8' (0x25)
        {.key = 0x0025, .value = '8'}, {.key = 0x0125, .value = '8'}, {.key = 0x0225, .value = '*'}, {.key = 0x0325, .value = '*'},
        // '9' (0x26)
        {.key = 0x0026, .value = '9'}, {.key = 0x0126, .value = '9'}, {.key = 0x0226, .value = '('}, {.key = 0x0326, .value = '('},
        // '0' (0x27)
        {.key = 0x0027, .value = '0'}, {.key = 0x0127, .value = '0'}, {.key = 0x0227, .value = ')'}, {.key = 0x0327, .value = ')'},

        // Enter (0x28)
        {.key = 0x0028, .value = 0x0A}, {.key = 0x0128, .value = 0x0A}, {.key = 0x0228, .value = 0x0A}, {.key = 0x0328, .value = 0x0A},
        // Escape (0x29)
        {.key = 0x0029, .value = 0x1B}, {.key = 0x0129, .value = 0x1B}, {.key = 0x0229, .value = 0x1B}, {.key = 0x0329, .value = 0x1B},
        // Backspace (0x2A)
        {.key = 0x002A, .value = 0x08}, {.key = 0x012A, .value = 0x08}, {.key = 0x022A, .value = 0x08}, {.key = 0x032A, .value = 0x08},
        // Tab (0x2B)
        {.key = 0x002B, .value = 0x09}, {.key = 0x012B, .value = 0x09}, {.key = 0x022B, .value = 0x09}, {.key = 0x032B, .value = 0x09},
        // Space (0x2C)
        {.key = 0x002C, .value = ' '},  {.key = 0x012C, .value = ' '},  {.key = 0x022C, .value = ' '},  {.key = 0x032C, .value = ' '},

        // '-' (0x2D)
        {.key = 0x002D, .value = '-'}, {.key = 0x012D, .value = '-'}, {.key = 0x022D, .value = '_'}, {.key = 0x032D, .value = '_'},
        // '=' (0x2E)
        {.key = 0x002E, .value = '='}, {.key = 0x012E, .value = '='}, {.key = 0x022E, .value = '+'}, {.key = 0x032E, .value = '+'},
        // '[' (0x2F)
        {.key = 0x002F, .value = '['}, {.key = 0x012F, .value = '['}, {.key = 0x022F, .value = '{'}, {.key = 0x032F, .value = '{'},
        // ']' (0x30)
        {.key = 0x0030, .value = ']'}, {.key = 0x0130, .value = ']'}, {.key = 0x0230, .value = '}'}, {.key = 0x0330, .value = '}'},
        // US Backslash (0x31)
        {.key = 0x0031, .value = 0x5C}, {.key = 0x0131, .value = 0x5C}, {.key = 0x0231, .value = '|'}, {.key = 0x0331, .value = '|'},
        // Non-US hash (0x32)
        {.key = 0x0032, .value = '#'}, {.key = 0x0132, .value = '#'}, {.key = 0x0232, .value = '~'}, {.key = 0x0332, .value = '~'},
        // ';' (0x33)
        {.key = 0x0033, .value = ';'}, {.key = 0x0133, .value = ';'}, {.key = 0x0233, .value = ':'}, {.key = 0x0333, .value = ':'},
        // Apostrophe / double quote (0x34)
        {.key = 0x0034, .value = 0x27}, {.key = 0x0134, .value = 0x27}, {.key = 0x0234, .value = 0x22}, {.key = 0x0334, .value = 0x22},
        // '`' (0x35)
        {.key = 0x0035, .value = '`'}, {.key = 0x0135, .value = '`'}, {.key = 0x0235, .value = '~'}, {.key = 0x0335, .value = '~'},
        // ',' (0x36)
        {.key = 0x0036, .value = ','}, {.key = 0x0136, .value = ','}, {.key = 0x0236, .value = '<'}, {.key = 0x0336, .value = '<'},
        // '.' (0x37)
        {.key = 0x0037, .value = '.'}, {.key = 0x0137, .value = '.'}, {.key = 0x0237, .value = '>'}, {.key = 0x0337, .value = '>'},
        // '/' (0x38)
        {.key = 0x0038, .value = '/'}, {.key = 0x0138, .value = '/'}, {.key = 0x0238, .value = '?'}, {.key = 0x0338, .value = '?'},
        // Non-US backslash (0x64)
        {.key = 0x0064, .value = 0x5C}, {.key = 0x0164, .value = 0x5C}, {.key = 0x0264, .value = '|'}, {.key = 0x0364, .value = '|'}
    }
};
