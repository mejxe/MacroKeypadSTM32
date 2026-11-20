#include "buttons.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "utils.h"
#include <stdio.h>

// INTERNAL BUTTON HANDLING
void button_scan(GPIO_PinState *previous_state, uint16_t *button_ticks,
                 MacroKeyboard *MK, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                 uint32_t *last_ms, button_t *button)
{
  const uint16_t PRESS_REGISTER_THRESHOLD = 100;
  const uint16_t HOLD_REGISTER_THRESHOLD = 400;
  button->pressed = false;
  if (!debounce_ok(30, last_ms))
    return;

  GPIO_PinState current_state = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
  if (current_state == GPIO_PIN_RESET && current_state == *previous_state)
  {
    (*button_ticks)++;
  }
  else if ((current_state == GPIO_PIN_SET ||
            current_state == *previous_state) &&
           *button_ticks < PRESS_REGISTER_THRESHOLD)
  {
    *button_ticks = 0;
    button->held = false;
    button->pressed = false;
  }

  if (current_state == GPIO_PIN_SET && current_state == *previous_state)
  {
    if (*button_ticks > PRESS_REGISTER_THRESHOLD &&
        *button_ticks < HOLD_REGISTER_THRESHOLD)
    {
      *button_ticks = 0;
      button->pressed = true;
    }
    else
    {
      *button_ticks = 0;
      button->pressed = false;
      button->held = false;
    }
  }

  if (!button->held && *button_ticks >= HOLD_REGISTER_THRESHOLD)
  {
    *button_ticks = 0;
    button->pressed = false;
    button->held = true;
  }
  *previous_state = current_state;
}
void on_board_button_scan(MacroKeyboard *MK, button_t *on_board_button)
{
  static uint32_t last_ms = 0;
  static GPIO_PinState prev_state = GPIO_PIN_SET;
  static uint16_t button_ticks = 0;
  button_scan(&prev_state, &button_ticks, MK, button_GPIO_Port, button_Pin,
              &last_ms, on_board_button);
}
void encoder_button_scan(MacroKeyboard *MK, button_t *encoder_button)
{
  static uint32_t last_ms = 0;
  static GPIO_PinState prev_state = GPIO_PIN_SET;
  static uint16_t button_ticks = 0;
  button_scan(&prev_state, &button_ticks, MK, encoder_Button_GPIO_PORT,
              encoder_Button_Pin, &last_ms, encoder_button);
}

button_t internal_encoder_button = {
    .pressed = false, .held = false, .held_time = 0};
button_t internal_onboard_button = {
    .pressed = false, .held = false, .held_time = 0};

// BUTTON INTERFACES

Button encoder_button = {.internal_button = &internal_encoder_button,
                         .registered_state = Released,
                         .scan_func = encoder_button_scan};

Button onboard_button = {.internal_button = &internal_onboard_button,
                         .registered_state = Released,
                         .scan_func = on_board_button_scan};

Button *buttons[2] = {&encoder_button, &onboard_button};

void buttons_scan(MacroKeyboard *MK)
{
  for (int i = 0; i < 2; i++)
  {
    buttons[i]->scan_func(MK, buttons[i]->internal_button);
    if (buttons[i]->internal_button->pressed)
      buttons[i]->registered_state = Pressed;

    else if (!buttons[i]->internal_button->pressed &&
             !buttons[i]->internal_button->held)
      buttons[i]->registered_state = Released;
  }
}

void buttons_task(MacroKeyboard *MK)
{
  for (int i = 0; i < 2; i++)
  {
    if (buttons[i]->registered_state == Held &&
        buttons[i]->dispatched_function != buttons[i]->hold_func)
    {
      buttons[i]->dispatched_function = buttons[i]->hold_func;
    }
    else if (buttons[i]->registered_state == LongHeld &&
             buttons[i]->dispatched_function != buttons[i]->long_hold_func)
    {
      buttons[i]->dispatched_function = buttons[i]->long_hold_func;
    }
    else if (buttons[i]->registered_state == Pressed &&
             buttons[i]->dispatched_function != buttons[i]->press_func)
    {
      buttons[i]->dispatched_function = buttons[i]->press_func;
    }

    if (buttons[i]->registered_state == Released &&
        buttons[i]->dispatched_function != NULL)
    {
      buttons[i]->dispatched_function(MK);
      buttons[i]->dispatched_function = NULL;
    }
  }
}
void button_time_hold_task()
{
  const static uint8_t HOLD_TIME_TO_REGISTER = 100;
  for (int i = 0; i < 2; i++)
  {
    if (buttons[i]->internal_button->held &&
        buttons[i]->internal_button->held_time >= HOLD_TIME_TO_REGISTER)
    {
      buttons[i]->registered_state = LongHeld;
    }
    else if (buttons[i]->internal_button->held)
    {
      buttons[i]->registered_state = Held;
      buttons[i]->internal_button->held_time++;
    }
    else
    {
      buttons[i]->internal_button->held_time = 0;
    }
  }
}

void _placeholder_button_func(MacroKeyboard *MK) {}
