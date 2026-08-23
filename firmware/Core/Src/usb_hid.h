#ifndef USB_HID_H
#define USB_HID_H

#include "class/hid/hid.h"
#include "macros.h"
#include <stdint.h>

void usb_keyboard_send_report(uint8_t *keycode, uint8_t modifier);
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen);

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize);
void macro_task(MacroKeyboard *MK);

#endif /* USB_HID_H */
