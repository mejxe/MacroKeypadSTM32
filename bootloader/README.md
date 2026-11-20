# Macro Keypad bootloader

A minimal bootloader, made for this project, that supports flashing over USB-C.

## Features

- starts the main app, unless the flashing flag is set
- flashing flag is stored in the RTC backup register, so it survives reset, but not power-off
- can receive and store the firmware in a backup flash area
- verifies CRC checksum before copying the firmware to main memory
