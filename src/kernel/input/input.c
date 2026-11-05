// input.c
// ----------------------------------------
// Handle general inputs
#include "input.h"
#include "drv/keyboard.h"
#include "messaging/messaging.h"

// setup drivers
void input_init(void) {
  message_send_message(" input: init\n");
  if (keyboard_init() == 0)
	  return;
  message_send_message("ERR:input: No keyboard was detected! How the fuck are you gonna do anything now??? Ve vill halt the system...\n");
  __asm__ volatile ("cli");
  for (;;)
	  ;
}

static input_event_t input_queue[64];

static volatile uint32_t input_head = 0;
static volatile uint32_t input_tail = 0;

int input_push_event(const input_event_t *event) {
  uint32_t next = (input_head + 1) % 64;
  if (next == input_tail)
	  return -1; //queueueueueue is full

  input_queue[input_head] = *event;
  input_head = next;
  return 0;
}

int input_poll_event(input_event_t *event) {
  if (!event)
	  return -1;

  if (input_head == input_tail)
	  return 0;

  *event = input_queue[input_tail];

  input_tail = (input_tail + 1) % 64;

  return 1;
}
