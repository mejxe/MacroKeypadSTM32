use std::fmt::Display;

pub use thiserror::Error;
pub type Result<T> = std::result::Result<T, Error>;

pub mod flashing;
pub mod loader;
pub mod writer;

mod helpers;
mod structs;
mod usb_hid;

pub use crate::structs::Mode;
// constants

const MAGIC_NUMBER: [u8; 4] = [0x25, 0x07, 0xa0, 0x7a];

const MACRO_KEYBOARD_PID: u16 = 0x2507;
const MACRO_KEYBOARD_VID: u16 = 0xA07A;

const BOOTLOADER_PID: u16 = 0xB007;
const BOOTLOADER_VID: u16 = 0xA07A;

const FLASH_MACRO_MESSAGE: u8 = 0x01;
const FLASH_FIRMWARE_MESSAGE: u8 = 0x02;

// FIRMWARE PATH FOR UPLOADING
const PATH: &str = "./firmware/MacroKeyboard.bin";

#[derive(Debug, Error)]
pub enum Error {
    #[error("Config error: {0}")]
    ConfigError(String),
    #[error("Port error: {0}")]
    PortError(String, PortErrorType),
    #[error("Binary error: {0}")]
    BinaryError(String),
}
#[derive(Debug)]
pub enum PortErrorType {
    NoPortFound,
    Misc,
}
