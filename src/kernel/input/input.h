#ifndef INPUT_INPUT_H
#define INPUT_INPUT_H

#include "kernel_types.h"

typedef enum {
  INPUT_EVENT_KEY,
  INPUT_EVENT_MOUSE
} input_event_type_t;

typedef struct {
  input_event_type_t type;

  union {
    struct {
      uint16_t keycode;
      uint8_t pressed;
    } key;

    struct {
      int16_t x, y;
      int8_t wheel;
      uint8_t buttons;
    } mouse;
  };
} input_event_t;

void input_init(void);

int input_push_event(const input_event_t *event);
int input_poll_event(input_event_t *event);
#endif
