#ifndef FLASHER_H
#define FLASHER_H
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define MESSAGE_BUFSIZE 4096

typedef enum
{
  AppSector,
  UpdateBufSector,
} FlashSector;

typedef struct
{
  uint8_t *message_buf;
  uint32_t in_buffer;
  uint32_t flash_offset;

  bool flasher_set_up;
  FlashSector sector;

  CRC_HandleTypeDef *hcrc;
} Flasher_t;

bool finish_flash(Flasher_t *flasher, uint32_t expected_crc);

bool write_flash(Flasher_t *flasher, const uint8_t *buf, uint32_t len);

bool setup_flasher(Flasher_t *f, FlashSector f_sector);

#endif /* FLASHER_H */
