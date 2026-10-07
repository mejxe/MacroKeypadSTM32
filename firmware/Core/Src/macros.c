#include "macros.h"
#include "encoder.h"
#include "flash.h"
#include "stm32f4xx_hal.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define MAX_KEYCODE = 0x28

void increment_switch_counter(KeySwitch *key) { key->down_counter += 1; }
void reset_switch_counter(KeySwitch *key) { key->down_counter = 0; }

bool should_register_keypress(KeySwitch *key)
{
  return (key->down_counter >= 8);
}

/*
 * _Format_
 * [macro_len, macro_modifier, 0, 0, [macro_len characters padded to
 * MAX_MACRO_LEN with zeroes]]
 */

void init_macro(Macro *macro_slot, uint8_t *macro, Modifier modifier,
                uint8_t macro_len)
{
  if (macro_len > MAX_MACRO_LEN)
  {
    return;
  }
  macro_slot->macro_len = macro_len;
  macro_slot->modifier = modifier;
  memcpy(macro_slot->macro, macro, macro_len * sizeof(uint8_t));
};
void default_setup_macros(MacroKeyboard *MK_Struct)
{
  uint8_t macro1[MAX_MACRO_LEN] = {KEY_0, KEY_0};
  uint8_t macro2[MAX_MACRO_LEN] = {KEY_0, KEY_1};
  uint8_t macro3[MAX_MACRO_LEN] = {KEY_0, KEY_2};
  uint8_t macro4[MAX_MACRO_LEN] = {KEY_1, KEY_0};
  uint8_t macro5[MAX_MACRO_LEN] = {KEY_1, KEY_1};
  uint8_t macro6[MAX_MACRO_LEN] = {KEY_1, KEY_2};
  uint8_t macro7[MAX_MACRO_LEN] = {KEY_2, KEY_0};
  uint8_t macro8[MAX_MACRO_LEN] = {KEY_2, KEY_1};
  uint8_t macro9[MAX_MACRO_LEN] = {KEY_2, KEY_2};

  uint8_t macro_left[MAX_MACRO_LEN] = {KEY_0};
  uint8_t macro_right[MAX_MACRO_LEN] = {KEY_1};

  init_macro(&MK_Struct->Macros_Array[0][0][0], macro1, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[0][1][0], macro2, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[0][2][0], macro3, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[1][0][0], macro4, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[1][1][0], macro5, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[1][2][0], macro6, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[2][0][0], macro7, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[2][1][0], macro8, NONE, 2);
  init_macro(&MK_Struct->Macros_Array[2][2][0], macro9, NONE, 2);

  init_macro(&MK_Struct->Encoder_Macros[DIRECTION_LEFT][0], macro_left, NONE,
             1);
  init_macro(&MK_Struct->Encoder_Macros[DIRECTION_RIGHT][0], macro_right, NONE,
             1);
}
void setup_macros(MacroKeyboard *MK)
{
  // old_setup_macros(MK);
  setup_macros_from_memory(MK);
}
Macro *get_macro_by_row_col(MacroKeyboard *MK_Struct, uint8_t row_index,
                            uint8_t col_index)
{
  return &MK_Struct
              ->Macros_Array[row_index][col_index][MK_Struct->macro_profile];
}

void delegate_macro(MacroKeyboard *MK_Struct, uint8_t row, uint8_t column)
{
  Macro *fetched_macro = get_macro_by_row_col(MK_Struct, row, column);
  if (fetched_macro == NULL)
  {
    return;
  }
  MK_Struct->macro_To_Send = fetched_macro;
}
void keyboard_scan(MacroKeyboard *MK_Struct)
{
  for (int i = 0; i < 3; i++)
  {
    HAL_GPIO_WritePin(GPIOB, MK_Struct->columns[i],
                      GPIO_PIN_RESET); // set to logical high
    for (int j = 0; j < 3; j++)
    {
      GPIO_PinState row_in = HAL_GPIO_ReadPin(GPIOB, MK_Struct->rows[j]);
      if (row_in == GPIO_PIN_RESET)
      {
        increment_switch_counter(&MK_Struct->Keyboard_State_Array[j][i]);
      }
      else if (row_in == GPIO_PIN_SET &&
               should_register_keypress(&MK_Struct->Keyboard_State_Array[j][i]))
      {
        delegate_macro(MK_Struct, j, i);
        reset_switch_counter(&MK_Struct->Keyboard_State_Array[j][i]);
      }
      else
      {
        reset_switch_counter(&MK_Struct->Keyboard_State_Array[j][i]);
      }
    }
    HAL_GPIO_WritePin(GPIOB, MK_Struct->columns[i],
                      GPIO_PIN_SET); // set to logical low
  }
}
void cycle_profiles(MacroKeyboard *MK_Struct)
{
  if (MK_Struct->macro_profile == MK_Struct->active_profiles - 1)
  {
    MK_Struct->macro_profile = 0;
    return;
  }
  MK_Struct->macro_profile++;
}

//
// debug methods
//
int32_t print_macro(Macro macro, char *buf, uint32_t bufsize)
{
  int32_t used_buf = 0;
  used_buf +=
      snprintf(buf + used_buf, bufsize - used_buf, "len: %d, modifier: %x\r\n",
               macro.macro_len, macro.modifier) +
      1;
  used_buf += snprintf(buf + used_buf, bufsize - used_buf, "macro: ") + 1;
  for (int i = 0; i < macro.macro_len; i++)
  {
    used_buf += snprintf(buf + used_buf, bufsize - used_buf, "%c ",
                         keycode_to_char_lookup(macro.macro[i])) +
                1;
  }
  used_buf += snprintf(buf + used_buf, bufsize - used_buf, "\r\n") + 1;
  return used_buf;
}

void print_macro_array(MacroKeyboard *MK_Struct)
{
  uint32_t bufsize = 2048;
  char buf[bufsize];
  uint32_t used_up = 0;
  used_up += snprintf(buf + used_up, bufsize - used_up, "MacroArray {\r\n") + 1;
  for (int p = 0; p < MACRO_PROFILES; p++)
  {
    used_up +=
        snprintf(buf + used_up, bufsize - used_up, "PROFILE %d\r\n", p) + 1;
    for (int i = 0; i < KEEB_ROWS; i++)
    {
      for (int j = 0; j < KEEB_COLS; j++)
      {
        used_up += snprintf(buf + used_up, bufsize - used_up,
                            "Macro r: %d c: %d p: %d {", i, j, p) +
                   1;
        used_up += print_macro(MK_Struct->Macros_Array[i][j][p], buf + used_up,
                               bufsize - used_up);
        used_up += snprintf(buf + used_up, bufsize - used_up, "}\r\n") + 1;
      }
    }
    for (int e = 0; e < 2; e++)
    {
      used_up += snprintf(buf + used_up, bufsize - used_up,
                          "Encoder Macro dir: %d p: %d {", e, p) +
                 1;
      used_up += print_macro(MK_Struct->Encoder_Macros[e][p], buf + used_up,
                             bufsize - used_up);
      used_up += snprintf(buf + used_up, bufsize - used_up, "}\r\n") + 1;
    }
  }
  used_up += snprintf(buf + used_up, bufsize - used_up, "}\r\n") + 1;
  print_buffered(buf, used_up);
}
