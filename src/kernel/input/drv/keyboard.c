// keyboard.c

// Watch me write an em dash
// — ooh em dash im an AI oooh AI whatever idk

// "It is better to shit in the sink than sink in the shit"
//                                              -Son Tzu
#include "kernel_types.h"
#include "messaging/messaging.h"
#include "input/input.h"

// drivers
#include "8402_kbd.h"

struct {
  union {
  struct __attribute__((packed)) {
    bool_t lshift: 1;
    bool_t lctrl : 1;
    bool_t rshift: 1;
    bool_t rctrl: 1;
    bool_t lmeta: 1;
    bool_t rmeta: 1;
    bool_t caps_lock: 1;
    bool_t spare: 1;
  } bools;
  uint8_t num;
  } modifiers;
  bool_t numlock;
  bool_t scrlock;
  uint32_t spare;
} input_keyboard_context;

void keyboard_queue_keystroke(uint16_t keycode, uint8_t pressed) {
  input_event_t x;
  x.type = INPUT_EVENT_KEY;
  x.key.keycode = keycode;
  x.key.pressed = pressed;
  input_push_event(&x);
}

int keyboard_init(void) {
  message_send_message(" keyboard: init\n");
  input_keyboard_context.modifiers.num = 0;
  input_keyboard_context.numlock = false;
  input_keyboard_context.scrlock = false;
  input_keyboard_context.spare = 0;
  if (kbd_8042_init() == 0)
	  return 0;
  message_send_message(" Keyboard: no keyboard found\n");
  return 1;
}
