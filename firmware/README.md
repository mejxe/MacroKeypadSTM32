# Macro Keypad firmware

This is a short description of the project's main functionality and features.

## Implementation

- Written in C
- STM-provided HAL
- Uses TinyUSB for the USB stack

## Features

- 9 keys, each one can hold a sequence of up to 16 characters
- macros can have modifiers (ctrls, alts, shifts, etc.)
- macros on encoder left/right rotation
- up to 3 layers of macros, cycled through by pressing the encoder
- macros survive power-off (stored in flash)
- macros programmable through USB-C
- macros in flash can be reset by holding the encoder button for 10s

## Modules

### usb
USB handling, descriptors and configuration through TinyUSB.

### macros
God struct definition, macro handling, helper functions for macros and macro keyboard.

### button
Buttons (encoder and onboard button) handling, also encoder handling.

### flash
Flash memory handling.
