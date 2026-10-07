#ifndef BUTTONS_H
#define BUTTONS_H

#include "macros.h"
typedef struct
{
  bool pressed;
  bool held;
  uint8_t held_time;
} button_t;

typedef enum
{
  Released = 0,
  Pressed = 1,
  Held = 2,
  LongHeld = 3,
} ButtonState;

typedef struct
{
  button_t *internal_button;
  ButtonState registered_state;
  void (*dispatched_function)(MacroKeyboard *);
  void (*press_func)(MacroKeyboard *);
  void (*hold_func)(MacroKeyboard *);
  void (*long_hold_func)(MacroKeyboard *);
  void (*scan_func)(MacroKeyboard *, button_t *);
} Button;

extern Button encoder_button;
extern Button onboard_button;
extern Button *buttons[2];

void buttons_scan(MacroKeyboard *MK);

void buttons_task(MacroKeyboard *MK);
void button_time_hold_task();

void _placeholder_button_func(MacroKeyboard *MK);

#endif /* BUTTONS_H */
