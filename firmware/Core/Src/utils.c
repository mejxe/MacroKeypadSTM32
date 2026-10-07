#include "utils.h"
#include "class/cdc/cdc_device.h"
#include "device/usbd.h"
#include "main.h"
#include <stdio.h>

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

void print_buffered(char *buf, uint32_t bufsize)
{
  char *p = buf;
  while (bufsize != 0)
  {
    if (tud_cdc_connected())
    {
      uint32_t avail = tud_cdc_write_available();
      if (avail == 0)
      {
        tud_cdc_write_flush();
        tud_task();
      }
      uint32_t wrote = tud_cdc_write(p, bufsize > avail ? avail : bufsize);
      tud_cdc_write_flush();
      bufsize -= wrote;
      p += wrote;
    }
  }
}
