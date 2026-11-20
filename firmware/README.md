# Macro Keypad firmware

This is the short description of the project's main functionality and features.

## Implementation

- Written in c
- STM-provided HAL
- Uses tinyusb for the usb stack

## Features

- 9 keys, each one can hold up to 16 character sequences
- macros can have modifiers (ctrls, alts, shifts, etc.)
- macro on the encoder left/right rotations
- up to 3 layers of macros, toggled through by pressing the encoder
- macros survive power-off (stored in flash)
- macros programmable through usbc
- macros in flash can be reset by holding encoder button for 10s

## Modules

### usb
USB handling, descriptors and configuration through tinyusb.

### macros
God struct definition, macro handling, helper functions for macros and macro keyboard.

### button
Buttons (encoder and onboard button) handling, also encoder handling.

### flash
Flash memory handling.
