// 8042_kbd.c
// --------------------------
// The 8042 microcontroller, otherwise known as...
//
// The P/S2 KEYBOARD CONTROLLER ASHIAHSFIASHFASKFHASKJFH SO FUNNY AHAHAHAHAHAHA
#include "idt/idt.h"
#include "input/input.h"
#include "kernel_ports.h"
#include "kernel_types.h"
#include "messaging/messaging.h"

// keyboard
#include "keyboard.h"

// ＊P/S2 is Personal system 2, not PS2 you shitheads

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

#define CMD_READ_CONFIG 0x20
#define CMD_SELF_TEST 0xAA
#define CMD_DISABLE_PORT1 0xAD
#define CMD_ENABLE_PORT1 0xAE
#define CMD_WRITE_CONFIG 0x60

#define KBD_SET_SCANCODE 0xF0
#define KBD_ENABLE 0xF4
#define KBD_ACK 0xFA
#define KBD_RESEND 0xFE

#define TEST_SUCCESS 0x55

static struct {
  uint8_t extended;
  uint8_t break_code;
} context_of_fucking_8042;

extern struct {
  union {
    struct __attribute__((packed)) {
      bool_t lshift : 1;
      bool_t lctrl : 1;
      bool_t rshift : 1;
      bool_t rctrl : 1;
      bool_t lmeta : 1;
      bool_t rmeta : 1;
      bool_t caps_lock : 1;
      bool_t spare : 1;
    } bools;
    uint8_t num;
  } modifiers;
  bool_t numlock;
  bool_t scrlock;
  uint32_t spare;
} input_keyboard_context;

// AI slopped conversion table
// im too lazy to look ts up
static const uint8_t ps2_set2_to_hid[256] = {
    0x00,
    0x42,
    0x00,
    0x3E,
    0x3C,
    0x3A,
    0x3B,
    0x45, // 0x00 - 0x07 (F9, F5, F3, F1, F2, F12)
    0x00,
    0x43,
    0x41,
    0x3F,
    0x3D,
    0x2B,
    0x35,
    0x00, // 0x08 - 0x0F (F10, F8, F6, F4, Tab, `)
    0x00,
    0xE2,
    0xE1,
    0x00,
    0xE0,
    0x14,
    0x1E,
    0x00, // 0x10 - 0x17 (LAlt, LShift, LCtrl, Q, 1)
    0x00,
    0x00,
    0x1D,
    0x16,
    0x04,
    0x1A,
    0x1F,
    0x00, // 0x18 - 0x1F (Z, S, A, W, 2)
    0x00,
    0x06,
    0x1B,
    0x07,
    0x08,
    0x21,
    0x20,
    0x00, // 0x20 - 0x27 (C, X, D, E, 4, 3)
    0x00,
    0x2C,
    0x19,
    0x09,
    0x17,
    0x15,
    0x22,
    0x00, // 0x28 - 0x2F (Space, V, F, T, R, 5)
    0x00,
    0x11,
    0x05,
    0x0B,
    0x0A,
    0x1C,
    0x23,
    0x00, // 0x30 - 0x37 (N, B, H, G, Y, 6)
    0x00,
    0x00,
    0x10,
    0x0D,
    0x18,
    0x24,
    0x25,
    0x00, // 0x38 - 0x3F (M, J, U, 7, 8)
    0x00,
    0x36,
    0x0E,
    0x0C,
    0x12,
    0x27,
    0x26,
    0x00, // 0x40 - 0x47 (, , K, I, O, 0, 9)
    0x00,
    0x37,
    0x38,
    0x0F,
    0x33,
    0x13,
    0x2D,
    0x00, // 0x48 - 0x4F (. , / , L, ;, P, -)
    0x00,
    0x00,
    0x34,
    0x00,
    0x2F,
    0x2E,
    0x00,
    0x00, // 0x50 - 0x57 (' , [ , =)
    0x00,
    0xE5,
    0x28,
    0x30,
    0x00,
    0x31,
    0x00,
    0x00, // 0x58 - 0x5F (RShift, Enter, ], \)
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x2A,
    0x00, // 0x60 - 0x67 (Backspace)
    0x00,
    0x59,
    0x00,
    0x5C,
    0x5F,
    0x00,
    0x00,
    0x00, // 0x68 - 0x6F (KP 1, KP 4, KP 7)
    0x62,
    0x63,
    0x5A,
    0x5D,
    0x5E,
    0x60,
    0x29,
    0x53, // 0x70 - 0x77 (KP 0, KP ., KP 2, KP 5, KP 6, KP 8, Esc,
          // NumLock)
    0x44,
    0x56,
    0x5B,
    0x55,
    0x54,
    0x61,
    0x00,
    0x00, // 0x78 - 0x7F (F11, KP +, KP 3, KP -, KP *, KP 9)
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
    0x00,
    0x00, // 0x80 - 0x87 (F7)
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0x88 - 0x8F
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0x90 - 0x97
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0x98 - 0x9F
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xA0 - 0xA7
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xA8 - 0xAF
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xB0 - 0xB7
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xB8 - 0xBF
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xC0 - 0xC7
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xC8 - 0xCF
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xD0 - 0xD7
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xD8 - 0xDF
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xE0 - 0xE7
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xE8 - 0xEF
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xF0 - 0xF7
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // 0xF8 - 0xFF
};

static const uint8_t ps2_set2_ext_to_hid[256] = {
    // Only mapping common physical keys
    [0x11] = 0xE6, // Right Alt
    [0x14] = 0xE4, // Right Ctrl
    [0x1F] = 0xE3, // Left GUI / Windows
    [0x27] = 0xE7, // Right GUI / Windows
    [0x2F] = 0x65, // Application / Menu
    [0x4A] = 0x54, // KP / (Divide)
    [0x5A] = 0x58, // KP Enter
    [0x6C] = 0x4A, // Home
    [0x6B] = 0x50, // Left Arrow
    [0x69] = 0x4D, // End
    [0x75] = 0x52, // Up Arrow
    [0x72] = 0x51, // Down Arrow
    [0x7D] = 0x4B, // Page Up
    [0x7A] = 0x4E, // Page Down
    [0x70] = 0x49, // Insert
    [0x71] = 0x4C, // Delete
    [0x74] = 0x4F, // Right Arrow
};

static inline void io_delay() { inb(0x80); }
static bool_t ps2_wait_input_clear(void) {
  uint32_t timeout = 100000;

  while (timeout--) {
    if (!(inb(PS2_STATUS_PORT) & 0x02))
      return true;

    io_delay();
  }

  return false;
}

static bool_t ps2_wait_output_full(void) {
  uint32_t timeout = 100000;

  while (timeout--) {
    if (inb(PS2_STATUS_PORT) & 0x01)
      return true;

    io_delay();
  }

  return false;
}

static bool_t ps2_controller_write(uint8_t value) {
  if (!ps2_wait_input_clear())
    return false;

  outb(PS2_CMD_PORT, value);
  return true;
}

static bool_t ps2_data_write(uint8_t value) {
  if (!ps2_wait_input_clear())
    return false;

  outb(PS2_DATA_PORT, value);
  return true;
}

static bool_t ps2_keyboard_command(uint8_t command) {
  for (int retry = 0; retry < 3; retry++) {
    if (!ps2_data_write(command))
      return false;

    if (!ps2_wait_output_full())
      return false;

    uint8_t response = inb(PS2_DATA_PORT);

    if (response == KBD_ACK)
      return true;

    if (response != KBD_RESEND)
      return false;
  }

  return false;
}

void kbd_8042_isr_callback(uint8_t isr_n) {
  uint8_t status = inb(PS2_STATUS_PORT);

  if (!(status & 0x1))
    return;
  uint8_t scancode = inb(PS2_DATA_PORT);
  if (scancode == 0xE0) {
    context_of_fucking_8042.extended = 1;
    return;
  }

  if (scancode == 0xF0) {
    context_of_fucking_8042.break_code = 1;
    return;
  }

  bool_t pressed = !context_of_fucking_8042.break_code;

  uint8_t hid;

  if (context_of_fucking_8042.extended)
    hid = ps2_set2_ext_to_hid[scancode];
  else
    hid = ps2_set2_to_hid[scancode];

  context_of_fucking_8042.extended = 0;
  context_of_fucking_8042.break_code = 0;

  if (!hid)
    return;

  keyboard_queue_keystroke(hid, pressed);

  // EOI handled by idt.
}

static bool_t probe_8042_no_acpi(void) {
  uint8_t status = inb(PS2_STATUS_PORT);
  if (status == 0xFF) {
    return false;
  }

  uint32_t timeout = 100000;
  while ((inb(PS2_STATUS_PORT) & 1) && timeout > 0) {
    inb(PS2_DATA_PORT);
    io_delay();
    timeout--;
  }
  if (timeout == 0)
    return false;

  timeout = 100000;
  while ((inb(PS2_STATUS_PORT) & 2) && timeout > 0) {
    io_delay();
    timeout--;
  }
  if (timeout == 0)
    return false;

  outb(PS2_CMD_PORT, CMD_READ_CONFIG);

  timeout = 100000;
  while (!(inb(PS2_STATUS_PORT) & 1) && timeout > 0) {
    io_delay();
    timeout--;
  }

  if (timeout == 0) {
    return false;
  }

  uint8_t config_byte = inb(PS2_DATA_PORT);

  if (config_byte == 0xFF) {
    return false;
  }

  return true;
}

int kbd_8042_init(void) {
  message_send_message(" 8042_kbd: init\n");
  if (!probe_8042_no_acpi())
    return 1;
  message_send_message(" 8042_kbd: interrupts will be temporarily halted...\n");
  __asm__ volatile("cli"); // interrupts go bye bye
  if (!ps2_controller_write(CMD_DISABLE_PORT1))
    goto fail;

  if (!ps2_controller_write(CMD_READ_CONFIG))
    goto fail;
  if (!ps2_wait_output_full())
    goto fail;
  uint8_t config = inb(PS2_DATA_PORT);
  // temporarily disable IRQ1 and set TL
  config &= ~(1 << 0);
  config &= ~(1 << 6);
  config |= (1 << 4);
  if (!ps2_controller_write(CMD_WRITE_CONFIG))
    goto fail;
  if (!ps2_data_write(config))
    goto fail;
  if (!ps2_controller_write(CMD_ENABLE_PORT1))
    goto fail;

  if (!ps2_keyboard_command(KBD_SET_SCANCODE))
    goto fail;
  if (!ps2_keyboard_command(0x02))
    goto fail;
  if (!ps2_keyboard_command(KBD_ENABLE))
    goto fail;
  while (inb(PS2_STATUS_PORT) & 0x01)
    inb(PS2_DATA_PORT); // discard the rest
  config &= ~(1 << 4);
  config |= (1 << 0);
  if (!ps2_controller_write(CMD_WRITE_CONFIG))
    goto fail;
  if (!ps2_data_write(config))
    goto fail;
  uint8_t mask = inb(0x21);
  outb(0x21, mask & ~(1 << 1)); // enable IRQ1
  idt_register_isr(kbd_8042_isr_callback, 1);
  __asm__ volatile("sti");
  message_send_message(" 8042_kbd: finish init\n");
  return 0;
fail:
  message_send_message("ERR:8042_kbd: Could not initialize P/S2 keyboard!!!\n");
  return 1;
}
