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
void format_macros_for_writing(MacroKeyboard *MK, uint8_t *buffer)
{
  uint32_t current_byte = 0;
  const uint32_t max_len = 2048;
  char buf[max_len];
  uint32_t used_up = 0;
  for (int p = 0; p < MACRO_PROFILES; p++)
  {
    used_up +=
        snprintf(buf + used_up, max_len - used_up, "PROFILE: %d\r\n", p) + 1;
    for (int i = 0; i < KEEB_ROWS; i++)
    {
      for (int j = 0; j < KEEB_COLS; j++)
      {
        used_up += snprintf(buf + used_up, max_len - used_up,
                            "macro: %d %d\r\n", i, j) +
                   1;
        uint8_t mlen = MK->Macros_Array[i][j][p].macro_len;
        Modifier mmod = MK->Macros_Array[i][j][p].modifier;
        buffer[current_byte++] = mlen;
        buffer[current_byte++] = mmod;
        used_up += snprintf(buf + used_up, max_len - used_up,
                            "macro_len: %d, macro_mod:  %d\r\n", mlen, mmod) +
                   1;
        buffer[current_byte++] = 0;
        buffer[current_byte++] = 0;
        for (int k = 0; k < MAX_MACRO_LEN; k++)
        {
          char macro_key =
              keycode_to_char_lookup(MK->Macros_Array[i][j][p].macro[k]);
          buffer[current_byte++] = MK->Macros_Array[i][j][p].macro[k];
          used_up +=
              snprintf(buf + used_up, max_len - used_up, "%c", macro_key) + 1;
        }
        used_up += snprintf(buf + used_up, max_len - used_up, "\r\n") + 1;
      }
    }
  }
  // print_buffered(buf, used_up);
}
void init_macro(uint8_t row, uint8_t col, uint8_t macro_profile, uint8_t *macro,
                Modifier modifier, uint8_t macro_len, MacroKeyboard *MK_Struct)
{
  if (macro_len > MAX_MACRO_LEN || macro_profile > MACRO_PROFILES)
  {
    return;
  }
  MK_Struct->Macros_Array[row][col][macro_profile] =
      (Macro){macro_len, modifier};
  memcpy(MK_Struct->Macros_Array[row][col][macro_profile].macro, macro,
         macro_len * sizeof(uint8_t));
};
void init_encoder_macro(ROTATION_DIRECTION direction, uint8_t *macro,
                        Modifier modifier, uint8_t macro_len,
                        MacroKeyboard *MK_Struct)
{
  if (macro_len > MAX_MACRO_LEN)
  {
    return;
  }
  MK_Struct->Encoder_Macros[direction] = (Macro){macro_len, modifier};
  memcpy(MK_Struct->Encoder_Macros[direction].macro, macro,
         macro_len * sizeof(uint8_t));
};
void old_setup_macros(MacroKeyboard *MK_Struct)
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
  uint8_t macro10[MAX_MACRO_LEN] = {KEY_0, KEY_0, KEY_1};

  uint8_t macro_left[MAX_MACRO_LEN] = {KEY_0};
  uint8_t macro_right[MAX_MACRO_LEN] = {KEY_1};

  init_macro(0, 0, 0, macro1, NONE, 2, MK_Struct);
  init_macro(0, 0, 1, macro10, NONE, 3, MK_Struct);
  init_macro(0, 1, 0, macro2, NONE, 2, MK_Struct);
  init_macro(0, 2, 0, macro3, NONE, 2, MK_Struct);
  init_macro(1, 0, 0, macro4, NONE, 2, MK_Struct);
  init_macro(1, 1, 0, macro5, NONE, 2, MK_Struct);
  init_macro(1, 2, 0, macro6, NONE, 2, MK_Struct);
  init_macro(2, 0, 0, macro7, NONE, 2, MK_Struct);
  init_macro(2, 1, 0, macro8, NONE, 2, MK_Struct);
  init_macro(2, 2, 0, macro9, NONE, 2, MK_Struct);

  init_encoder_macro(DIRECTION_LEFT, macro_left, NONE, 1, MK_Struct);
  init_encoder_macro(DIRECTION_RIGHT, macro_right, NONE, 1, MK_Struct);
}
void setup_macros(MacroKeyboard *MK) { setup_macros_from_memory(MK); }
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
  if (MK_Struct->macro_profile == MACRO_PROFILES - 1)
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
  }
  used_up += snprintf(buf + used_up, bufsize - used_up, "}\r\n") + 1;
  print_buffered(buf, used_up);
}
