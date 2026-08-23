#include "macros.h"
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
void setup_macros(MacroKeyboard *MK_Struct)
{
  uint8_t macro1[MAX_MACRO_LEN] = {KEY_P, KEY_P, KEY_P, KEY_P,
                                   KEY_I, KEY_W, KEY_O};
  uint8_t macro2[MAX_MACRO_LEN] = {KEY_U, KEY_M, KEY_Y, KEY_J};
  uint8_t macro3[MAX_MACRO_LEN] = {KEY_Q};
  uint8_t macro4[MAX_MACRO_LEN] = {KEY_C, KEY_H, KEY_U, KEY_J};

  init_macro(0, 0, macro1, LSHIFT, 7, MK_Struct);
  init_macro(0, 1, macro2, NONE, 4, MK_Struct);
  init_macro(1, 0, macro3, LSUPER, 1, MK_Struct);
  init_macro(1, 1, macro4, NONE, 4, MK_Struct);
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
