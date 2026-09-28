#ifndef FLASH_H
#define FLASH_H
#include "macros.h"

#define MACRO_VECTOR_SECTOR_START_ADDRESS 0x08040000
#define BYTES_PER_MACRO (MAX_MACRO_LEN + 4)
#define ENCODER_MACROS 2
#define HEADER_BYTES 2
#define FLASH_MACRO_MEMORY_SIZE                                                \
  (KEEB_ROWS * KEEB_COLS + ENCODER_MACROS) * MACRO_PROFILES *BYTES_PER_MACRO + \
      HEADER_BYTES

void setup_macros_from_memory(MacroKeyboard *MK);

uint8_t GetSector(uint32_t StartSectorAddress);
uint32_t Flash_Write_Data(uint32_t StartSectorAddress, MacroKeyboard *MK);

void format_macros_for_writing(MacroKeyboard *MK, uint8_t *buffer);

void read_macros_from_flash(uint32_t start_addr, MacroKeyboard *MK_Struct);
void read_macros_from_buf(uint8_t *buffer, MacroKeyboard *MK_Struct);

#endif /* FLASH_H */
