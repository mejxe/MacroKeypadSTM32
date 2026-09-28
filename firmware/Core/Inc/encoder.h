#ifndef ENCODER_H
#define ENCODER_H

#include "macros.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum
{
  // 1 is logical low
  low_low = 0b11,
  high_low = 0b01,
  high_high = 0b00,
  low_high = 0b10,
} ENCODER_STATE;

typedef enum
{
  DIRECTION_LEFT = 0,
  DIRECTION_RIGHT = 1,
  DIRECTION_NONE = 2,
} ROTATION_DIRECTION;
static const uint8_t encoder_order_to_state_right[0b101] = {
    [low_low] = 0,
    [high_low] = 1,
    [high_high] = 2,
    [low_high] = 3,
};

static const uint8_t encoder_order_to_state_left[0b101] = {
    [low_low] = 0,
    [low_high] = 1,
    [high_high] = 2,
    [high_low] = 3,
};

typedef struct
{
  ENCODER_STATE previous_state;
  ROTATION_DIRECTION direction;
} Encoder;

void encoder_scan(Encoder *encoder, MacroKeyboard *MK);

#endif /* ENCODER_H */
