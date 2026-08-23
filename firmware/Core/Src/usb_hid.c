#include "usb_hid.h"
#include "class/hid/hid_device.h"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "usb_descriptors.h"
#include "utils.h"
#include <stdio.h>

void usb_keyboard_send_report(uint8_t *keycode, uint8_t modifier)
{
  uint8_t const report_id = 0;
  tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, report_id, modifier, keycode);
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); // blink the diode while sending
  printf("sending key: %c and modifier: %x!\r\n",
         keycode_to_char_lookup(keycode[0]), modifier);
}
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
  // TODO not Implemented
  (void)itf;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
  // This example doesn't use multiple report and report ID
  (void)itf;
  (void)report_id;
  (void)report_type;

  // echo back anything we received from host
  tud_hid_report(0, buffer, bufsize);
}
void macro_task(MacroKeyboard *MK)
{
  if (!tud_hid_n_ready(ITF_NUM_KEYBOARD) || MK->macro_To_Send == NULL)
  {
    // printf("returning because of: %d %d\r\n", tud_hid_n_ready(ITF_NUM_HID),
    //           MK->macro_To_Send);
    return;
  }
  uint8_t static macro_curr_index;
  bool static need_flush;

  uint8_t keycode[6] = {0};
  uint8_t modifier;

  if (need_flush) // flush with zeroes after every raport
  {
    keycode[0] = 0;
    modifier = 0;
    usb_keyboard_send_report(keycode, modifier);
    need_flush = false;
    macro_curr_index++;
    if (macro_curr_index >= MK->macro_To_Send->macro_len)
    {
      MK->macro_To_Send = NULL;
      macro_curr_index = 0;
    }
  }
  else
  {
    keycode[0] = MK->macro_To_Send->macro[macro_curr_index];
    modifier = MK->macro_To_Send->modifier;
    usb_keyboard_send_report(keycode, modifier);
    need_flush = true;
  }
}
