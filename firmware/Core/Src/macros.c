#include "macros.h"
#include "encoder.h"
#include "stm32f4xx_hal.h"
#include "utils.h"
#include <string.h>

#define MAX_KEYCODE = 0x28

void increment_switch_counter(KeySwitch *key) { key->down_counter += 1; }
void reset_switch_counter(KeySwitch *key) { key->down_counter = 0; }

bool should_register_keypress(KeySwitch *key)
{
  return (key->down_counter >= 8);
}
void format_macros_for_writing(Macro macros[3][3], uint8_t *buffer)
{
  uint8_t current_byte = 0;
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      buffer[current_byte++] = macros[i][j].macro_len;
      buffer[current_byte++] = macros[i][j].modifier;
      buffer[current_byte++] = 0;
      buffer[current_byte++] = 0;
      for (int k = 0; k < MAX_MACRO_LEN; k++)
      {
        buffer[current_byte++] = macros[i][j].macro[k];
      }
    }
  }
}
void init_macro(uint8_t row, uint8_t col, uint8_t *macro, Modifier modifier,
                uint8_t macro_len, MacroKeyboard *MK_Struct)
{
  if (macro_len > MAX_MACRO_LEN)
  {
    return;
  }
  MK_Struct->Macros_Array[row][col] = (Macro){macro_len, modifier};
  memcpy(MK_Struct->Macros_Array[row][col].macro, macro,
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
void setup_macros(MacroKeyboard *MK_Struct)
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

  init_macro(0, 0, macro1, NONE, 2, MK_Struct);
  init_macro(0, 1, macro2, NONE, 2, MK_Struct);
  init_macro(0, 2, macro3, NONE, 2, MK_Struct);
  init_macro(1, 0, macro4, NONE, 2, MK_Struct);
  init_macro(1, 1, macro5, NONE, 2, MK_Struct);
  init_macro(1, 2, macro6, NONE, 2, MK_Struct);
  init_macro(2, 0, macro7, NONE, 2, MK_Struct);
  init_macro(2, 1, macro8, NONE, 2, MK_Struct);
  init_macro(2, 2, macro9, NONE, 2, MK_Struct);

  init_encoder_macro(DIRECTION_LEFT, macro_left, NONE, 1, MK_Struct);
  init_encoder_macro(DIRECTION_RIGHT, macro_right, NONE, 1, MK_Struct);
}
Macro *get_macro_by_row_col(MacroKeyboard *MK_Struct, uint8_t row_index,
                            uint8_t col_index)
{
  return &MK_Struct->Macros_Array[row_index][col_index];
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
