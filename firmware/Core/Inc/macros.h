#ifndef MACROS_H
#define MACROS_H

#include <stdbool.h>
#include <stdint.h>
#define MAX_KEYCODE = 0x28
#define MAX_MACRO_LEN 16
#define KEEB_ROWS 3
#define KEEB_COLS 3
#define MACRO_PROFILES 3

typedef enum
{
  NONE = 0,
  LCTRL = 0x01,
  LSHIFT = 0x02,
  LALT = 0x04,
  LSUPER = 0x08,
  RCTRL = 0x10,
  RSHIFT = 0x20,
  RALT = 0x40,
  RSUPER = 0x80,
} ModifierEnum;
typedef uint8_t Modifier;

typedef struct
{
  uint8_t down_counter;
  bool logical_state;
} KeySwitch;

typedef struct
{
  uint8_t macro_len;
  Modifier modifier;
  uint8_t macro[MAX_MACRO_LEN];
} Macro;

typedef struct
{
  KeySwitch Keyboard_State_Array[KEEB_ROWS][KEEB_COLS];
  Macro Macros_Array[KEEB_ROWS][KEEB_COLS][MACRO_PROFILES];
  Macro Encoder_Macros[2][MACRO_PROFILES];
  const uint16_t columns[KEEB_COLS];
  const uint16_t rows[KEEB_ROWS];
  Macro *macro_To_Send;
  uint8_t macro_profile;
  uint8_t active_profiles;

} MacroKeyboard;

void init_macro(Macro *macro_slot, uint8_t *macro, Modifier modifier,
                uint8_t macro_len);
Macro *get_macro_by_row_col(MacroKeyboard *MK_Struct, uint8_t row_index,
                            uint8_t col_index);

void delegate_macro(MacroKeyboard *MK_Struct, uint8_t row, uint8_t column);
void macro_task(MacroKeyboard *MK);
void setup_macros(MacroKeyboard *MK_Struct);
void increment_switch_counter(KeySwitch *key);
void reset_switch_counter(KeySwitch *key);

bool should_register_keypress(KeySwitch *key);

void keyboard_scan(MacroKeyboard *MK_Struct);
void cycle_profiles(MacroKeyboard *MK_Struct);

void print_macro_array(MacroKeyboard *MK_Struct);
int32_t print_macro(Macro macro, char *buf, uint32_t bufsize);
void default_setup_macros(MacroKeyboard *MK_Struct);
#endif /* MACROS_H */
