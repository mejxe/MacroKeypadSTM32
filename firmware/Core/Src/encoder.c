#include "encoder.h"
#include "main.h"
#include "utils.h"
void encoder_reset(Encoder *encoder)
{
  encoder->previous_state = low_low;
  encoder->direction = DIRECTION_NONE;
}

bool encoder_transition(Encoder *encoder, ENCODER_STATE current_state)
// returns true if final transition
{
  if (current_state == encoder->previous_state)
    return false;
  if (encoder->direction == DIRECTION_NONE)
  // first transition
  {
    if (encoder->previous_state == low_low && current_state == high_low)
    {
      encoder->previous_state = current_state;
      encoder->direction = DIRECTION_LEFT;
    }
    else if (encoder->previous_state == low_low && current_state == low_high)
    {
      encoder->previous_state = current_state;
      encoder->direction = DIRECTION_RIGHT;
    }
    return false;
  }
  uint8_t prev_state_index;
  uint8_t curr_state_index;
  switch (encoder->direction)
  {
  case DIRECTION_LEFT:
    prev_state_index = encoder_order_to_state_left[encoder->previous_state];
    curr_state_index = encoder_order_to_state_left[current_state];
    break;
  case DIRECTION_RIGHT:
    prev_state_index = encoder_order_to_state_right[encoder->previous_state];
    curr_state_index = encoder_order_to_state_right[current_state];
    break;
  default:
    encoder->direction = DIRECTION_NONE;
    return false;
  }
  if (prev_state_index == 3 && curr_state_index == 0)
  {
    // register rotation
    return true;
  }
  else if (curr_state_index - prev_state_index == 1 ||
           prev_state_index - curr_state_index == 1)
  {
    // go to next state
    encoder->previous_state = current_state;

    if (curr_state_index == 0)
      encoder_reset(encoder);
  }
  else
  {
    encoder_reset(encoder);
  }
  return false;
}
void encoder_scan(Encoder *encoder, MacroKeyboard *MK)
{
  GPIO_PinState pin_A = HAL_GPIO_ReadPin(encoder_GPIO_PORT, encoder_A_Pin);
  GPIO_PinState pin_B = HAL_GPIO_ReadPin(encoder_GPIO_PORT, encoder_B_Pin);
  ENCODER_STATE current_state = (pin_A << 1) | pin_B;
  bool did_rotate = encoder_transition(encoder, current_state);
  if (did_rotate)
  {
    switch (encoder->direction)
    {
    case DIRECTION_LEFT:
      MK->macro_To_Send = &MK->Encoder_Macros[DIRECTION_LEFT];
      break;
    case DIRECTION_RIGHT:
      MK->macro_To_Send = &MK->Encoder_Macros[DIRECTION_RIGHT];
      break;
    case DIRECTION_NONE:
      return; // illegal state btw, probably call error or smthing
    }
    encoder_reset(encoder);
  }
  return;
}
