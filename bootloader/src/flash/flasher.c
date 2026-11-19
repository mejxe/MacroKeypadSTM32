#include "flasher.h"
#include "flash_layout.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/// pfp
bool flasher_flush_buf(Flasher_t *flasher);
uint32_t flasher_get_sector(Flasher_t *f);
void *flasher_get_message_buf_pos(Flasher_t *f);
uint32_t write_buf_to_flash(uint32_t flash_address, uint32_t *buf,
                            uint32_t bufsize);
////
bool finish_flash(Flasher_t *flasher, uint32_t expected_crc)
{
  if (flasher->in_buffer != 0)
  {
    if (!flasher_flush_buf(flasher))
    {
      HAL_FLASH_Lock();
      return false;
    }
  }

  flasher->in_buffer = 0;
  flasher->flash_offset = 0;
  HAL_FLASH_Lock();

  return flasher->hcrc->Instance->DR == expected_crc;
}
void *flasher_get_message_buf_pos(Flasher_t *f)
{
  return (f->message_buf + f->in_buffer);
}

bool flasher_flush_buf(Flasher_t *flasher)
{

  uint32_t cast_buffer[MESSAGE_BUFSIZE / 4 + 1];
  uint32_t words_to_write = (flasher->in_buffer + 3) / 4;
  uint32_t TARGET_FLASH_BUFFER = flasher_get_sector(flasher);

  memset(cast_buffer, 0xFF, words_to_write * 4);
  memcpy(cast_buffer, flasher->message_buf, flasher->in_buffer);

  HAL_CRC_Accumulate(flasher->hcrc, cast_buffer, words_to_write);

  uint32_t words_written = write_buf_to_flash(
      TARGET_FLASH_BUFFER + flasher->flash_offset, cast_buffer, words_to_write);

  if (words_written != words_to_write)
    return false;

  flasher->flash_offset += words_written * 4;
  flasher->in_buffer = 0;
  return true;
}

bool write_flash(Flasher_t *flasher, const uint8_t *buf, uint32_t len)
{
  uint32_t copied = 0;
  while (copied != len)
  {
    uint32_t capacity = MESSAGE_BUFSIZE - flasher->in_buffer;
    uint32_t to_copy = len - copied > capacity ? capacity : len - copied;
    memcpy(flasher_get_message_buf_pos(flasher), buf + copied, to_copy);
    flasher->in_buffer += to_copy;
    copied += to_copy;

    if (flasher->in_buffer == MESSAGE_BUFSIZE)
    {
      if (!flasher_flush_buf(flasher))
        return false;
    }
  }
  return true;
}

uint32_t flasher_get_sector(Flasher_t *f)
{
  switch (f->sector)
  {
  case AppSector:
    return APP_SECTOR_START;
  case UpdateBufSector:
    return FLASH_UPDATE_BUF;
  default:
    return 0;
  }
}

bool setup_flasher(Flasher_t *f, FlashSector f_sector)
{
  static FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SECTORError;
  uint8_t sector;
  uint8_t nb_sectors;

  switch (f_sector)
  {
  case AppSector:
    sector = FLASH_SECTOR_4;
    nb_sectors = 3;
    break;
  case UpdateBufSector:
    sector = FLASH_SECTOR_7;
    nb_sectors = 1;
    break;
  default:
    return false;
  }
  HAL_FLASH_Unlock();

  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = sector;
  EraseInitStruct.NbSectors = nb_sectors;

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
  {
    HAL_FLASH_Lock();
    return false;
  }

  f->sector = f_sector;
  f->flasher_set_up = true;

  __HAL_CRC_DR_RESET(f->hcrc);
  return true;
}

uint32_t write_buf_to_flash(uint32_t flash_address, uint32_t *buf,
                            uint32_t bufsize)
{
  uint32_t number_of_words = bufsize;
  uint32_t written_words = 0;

  while (written_words < number_of_words)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address,
                          buf[written_words]) == HAL_OK)
    {
      written_words++;
      flash_address += sizeof(*buf);
    }
    else
    {
      HAL_FLASH_Lock();
      return 0;
    }
  }
  return written_words;
}
