#include "flash.h"
#include "stm32f4xx_hal.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
void setup_macros_from_memory(MacroKeyboard *MK)
{
  read_macros_from_flash(MACRO_VECTOR_SECTOR_START_ADDRESS, MK);
}

uint8_t GetSector(uint32_t StartSectorAddress)
{
  if (StartSectorAddress < 0x08004000)
    return FLASH_SECTOR_0;
  if (StartSectorAddress < 0x08008000)
    return FLASH_SECTOR_1;
  if (StartSectorAddress < 0x0800C000)
    return FLASH_SECTOR_2;
  if (StartSectorAddress < 0x08010000)
    return FLASH_SECTOR_3;
  if (StartSectorAddress < 0x08020000)
    return FLASH_SECTOR_4;
  if (StartSectorAddress < 0x08040000)
    return FLASH_SECTOR_5;
  if (StartSectorAddress < 0x08060000)
    return FLASH_SECTOR_6;
  return FLASH_SECTOR_7;
}
uint32_t Flash_Write_Data(uint32_t StartSectorAddress, MacroKeyboard *MK)
{
  const static uint32_t NUMBER_OF_WORDS = FLASH_MACRO_MEMORY_SIZE + 1;
  static FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SECTORError;
  uint32_t sofar = 0;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  uint32_t StartSector = GetSector(StartSectorAddress);
  uint32_t EndSectorAddress = StartSectorAddress + NUMBER_OF_WORDS;
  uint32_t EndSector = GetSector(EndSectorAddress);

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = StartSector;
  EraseInitStruct.NbSectors = (EndSector - StartSector) + 1;

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
  {
    HAL_FLASH_Lock();
    return HAL_FLASH_GetError();
  }

  uint8_t Data[FLASH_MACRO_MEMORY_SIZE] = {0};
  format_macros_for_writing(MK, Data);

  while (sofar < NUMBER_OF_WORDS)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, StartSectorAddress,
                          Data[sofar]) == HAL_OK)
    {
      StartSectorAddress += 1;
      sofar++;
    }
    else
    {
      /* Error occurred while writing data in Flash memory*/
      HAL_FLASH_Lock();
      return HAL_FLASH_GetError();
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return 0;
}

void deserialize_macro(uint8_t *buffer, uint32_t buf_pos, Macro *macro_slot)
{
  uint8_t macro_len = buffer[buf_pos++];
  uint8_t macro_modifier = buffer[buf_pos++];
  buf_pos += 2; // skip padding

  uint8_t macro[MAX_MACRO_LEN] = {0};
  for (int macro_b = 0; macro_b < MAX_MACRO_LEN; macro_b++)
  {
    macro[macro_b] = buffer[buf_pos++];
  }
  init_macro(macro_slot, macro, macro_modifier, macro_len);
}

void read_macros_from_buf(uint8_t *buffer, MacroKeyboard *MK_Struct)
{
  uint32_t buf_pos = 0;

  MK_Struct->active_profiles = buffer[buf_pos];
  buf_pos += 2;

  for (int p = 0; p < MACRO_PROFILES; p++)
  {
    for (int i = 0; i < KEEB_ROWS; i++)
    {
      for (int j = 0; j < KEEB_COLS; j++)
      {
        deserialize_macro(buffer, buf_pos, &MK_Struct->Macros_Array[i][j][p]);
        buf_pos += 20;
      }
    }

    for (int e = 0; e < 2; e++)
    {
      deserialize_macro(buffer, buf_pos, &MK_Struct->Encoder_Macros[e][p]);
      buf_pos += 20;
    }
  }
}
void read_macros_from_flash(uint32_t start_addr, MacroKeyboard *MK_Struct)
{
  uint8_t buffer[FLASH_MACRO_MEMORY_SIZE];
  memcpy(buffer, (void *)start_addr, sizeof(buffer));

  read_macros_from_buf(buffer, MK_Struct);
}

void serialize_macro(const Macro *macro, uint8_t *buffer, uint32_t current_byte)
{
  uint8_t mlen = macro->macro_len;
  Modifier mmod = macro->modifier;
  buffer[current_byte++] = mlen;
  buffer[current_byte++] = mmod;

  buffer[current_byte++] = 0;
  buffer[current_byte++] = 0;
  for (int k = 0; k < MAX_MACRO_LEN; k++)
  {
    buffer[current_byte++] = macro->macro[k];
  }
}

void format_macros_for_writing(MacroKeyboard *MK, uint8_t *buffer)
{
  uint32_t current_byte = 0;

  buffer[current_byte++] = MK->active_profiles;
  buffer[current_byte++] = 0;

  for (int p = 0; p < MACRO_PROFILES; p++)
  {
    for (int i = 0; i < KEEB_ROWS; i++)
    {
      for (int j = 0; j < KEEB_COLS; j++)
      {
        serialize_macro(&MK->Macros_Array[i][j][p], buffer, current_byte);
        current_byte += 20;
      }
    }
    // print_buffered(buf, used_up);
    for (int e = 0; e < 2; e++)
    {
      serialize_macro(&MK->Encoder_Macros[e][p], buffer, current_byte);
      current_byte += 20;
    }
  }
}

void reset_macros_and_flash(MacroKeyboard *MK)
{
  default_setup_macros(MK);
  printf("RESET MACROS AND FLASH TO DEFAULT\r\n");
  Flash_Write_Data(MACRO_VECTOR_SECTOR_START_ADDRESS, MK);
}
