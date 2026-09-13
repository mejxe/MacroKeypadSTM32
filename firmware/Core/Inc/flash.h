#ifndef FLASH_H
#define FLASH_H
#include "macros.h"

#define MACRO_VECTOR_SECTOR_START_ADDRESS 0x08040000
#define BYTES_PER_MACRO (MAX_MACRO_LEN + 4)
#define FLASH_MACRO_MEMORY_SIZE                                                \
  KEEB_ROWS * KEEB_COLS * MACRO_PROFILES *BYTES_PER_MACRO

void setup_macros_from_memory(MacroKeyboard *MK);

uint8_t GetSector(uint32_t StartSectorAddress);
uint32_t Flash_Write_Data(uint32_t StartSectorAddress, MacroKeyboard *MK);

void read_macros_from_flash(uint32_t start_addr, MacroKeyboard *MK_Struct);

#endif /* FLASH_H */
