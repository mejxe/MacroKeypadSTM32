# Macro Keypad programmer

A simple tool for flashing and programming macros.

## Programming macros

**Before**
- fill the config.toml
- do not exceed 16 chars per macro
- fill as much layers as you specify

**Then**
`cargo run macro`

## Flashing

**Before**
- obtain the firmware .bin file
- the path of the firmware should be: `./firmware/MacroKeypad.bin`

**Then**
`cargo run flash`

The tool does not provide any feedback after transmitting the full firmware.
You can inspect the cdc channel while flashing to see what is happening on the device.
