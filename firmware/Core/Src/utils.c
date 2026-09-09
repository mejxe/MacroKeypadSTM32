#include "utils.h"
#include "main.h"

const char keycode_names[] = {
    [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd', [KEY_E] = 'e',
    [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h', [KEY_I] = 'i', [KEY_J] = 'j',
    [KEY_K] = 'k', [KEY_L] = 'l', [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o',
    [KEY_P] = 'p', [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
    [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x', [KEY_Y] = 'y',
    [KEY_Z] = 'z', [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3', [KEY_0] = '0',
};
const uint8_t char_to_keycode[128] = {
    ['a'] = KEY_A, ['b'] = KEY_B, ['c'] = KEY_C, ['d'] = KEY_D, ['e'] = KEY_E,
    ['f'] = KEY_F, ['g'] = KEY_G, ['h'] = KEY_H, ['i'] = KEY_I, ['j'] = KEY_J,
    ['k'] = KEY_K, ['l'] = KEY_L, ['m'] = KEY_M, ['n'] = KEY_N, ['o'] = KEY_O,
    ['p'] = KEY_P, ['q'] = KEY_Q, ['r'] = KEY_R, ['s'] = KEY_S, ['t'] = KEY_T,
    ['u'] = KEY_U, ['v'] = KEY_V, ['w'] = KEY_W, ['x'] = KEY_X, ['y'] = KEY_Y,
    ['z'] = KEY_Z, ['0'] = KEY_0, ['1'] = KEY_1, ['2'] = KEY_2, ['3'] = KEY_3,
};
uint8_t char_to_keycode_lookup(char c)
{
  uint8_t idx = (uint8_t)c;
  return (idx < 128) ? char_to_keycode[idx] : 0; // 0 = "no key" sentinel
}
const char keycode_to_char_lookup(uint8_t keycode)
{
  return keycode_names[keycode];
}
bool delay_non_blocking_ms(uint32_t interval, uint32_t *start_ms)
{
  if (HAL_GetTick() - *start_ms < interval)
  {
    return false;
  }
  (*start_ms) += interval;
  return true;
}
bool debounce_ok(uint32_t interval, uint32_t *start_ms)
{
  if (HAL_GetTick() - *start_ms < interval)
  {
    return false;
  }
  *start_ms = interval;
  return true;
}

bool button_scan(GPIO_PinState *previous_state, uint8_t *button_ticks,
                 MacroKeyboard *MK, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                 uint32_t *last_ms, bool *held_down)
{
  bool pressed = false;

  if (!debounce_ok(30, last_ms))
    return pressed;

  GPIO_PinState current_state = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
  if (current_state == GPIO_PIN_RESET && current_state == *previous_state)
  {
    (*button_ticks)++;
  }
  else
  {
    *button_ticks = 0;
    *held_down = false;
  }
  if (!(*held_down) && *button_ticks == 3)
  {
    *button_ticks = 0;
    *held_down = true;
    pressed = true;
  }
  *previous_state = current_state;
  return pressed;
}
bool on_board_button_scan(MacroKeyboard *MK)
{
  static uint32_t last_ms = 0;
  static bool held_down = false;
  static GPIO_PinState prev_state = GPIO_PIN_SET;
  static uint8_t button_ticks = 0;
  return button_scan(&prev_state, &button_ticks, MK, button_GPIO_Port,
                     button_Pin, &last_ms, &held_down);
}
bool encoder_button_scan(MacroKeyboard *MK)
{
  static uint32_t last_ms = 0;
  static bool held_down = false;
  static GPIO_PinState prev_state = GPIO_PIN_SET;
  static uint8_t button_ticks = 0;
  return button_scan(&prev_state, &button_ticks, MK, encoder_Button_GPIO_PORT,
                     encoder_Button_Pin, &last_ms, &held_down);
}
