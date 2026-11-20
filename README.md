# Macro Keypad 

**Programmable Macro Keypad**
Allows for macros with length up to 16 chars for 9 keys on 3 layers.

Functional, core stuff works, but still some work to be done.

The project is written without an RTOS, just plain bare metal with STM HAL.

I also designed the PCB and ordered it online.

*This is my first embedded project and it is purely educational.*
*Feel free to critize it, I'm still learning.*

## Hardware

- STM32 blackpill 
- Some spare switches & keycaps I had lying around
- E12 Encoder
- Custom daughter board
- 1N4148 diodes in a SOD232 package

## Repo Structure

Repo consits of:
* firmware - main macro keypad logic and functionality
* bootloader - bootloader that allows for programming macros and flashing firmware through cdc
* programmer - host only tool for programming macros and flashing firmware
* c_libs - tinyusb src

## Finished product

![Macro Keypad](docs/photo.jpg)
*Yes, I'm aware the buttons are crooked, I misaligned it while soldering, mb.*

##### Disclaimer
No AI code generation was used for this project to enchance the learning process.
*It was hovewer, used for things like research and reading the datasheets to speed things up.*
