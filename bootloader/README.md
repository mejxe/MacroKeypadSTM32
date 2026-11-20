# Macro Keypad bootloader

A minimal bootloader to support flashing through usbc, that I made for the project.

## Features

- starts the main app, unless the flashing flag is set
- flashing flag is stored in the RTC Backup register, so it survives reset, but not poweroff
- can receive and store the firmware in a backup flash area
- verifies CRC hash before copying the firmware to main memory
