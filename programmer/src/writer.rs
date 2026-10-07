use std::time::Duration;

use crate::Error;
use crate::{
    Result,
    structs::{Macro, MacroConfig, MacroProfile, Modifiers, SerializedConfig},
    usb_hid::{char_to_hid_code, modifiers_to_hid_code},
};
const MAGIC_NUMBER: [u8; 4] = [0x25, 0x07, 0xa0, 0x7a];
const MACRO_KEYBOARD_PID: u16 = 0x2507;
const MACRO_KEYBOARD_VID: u16 = 0xA07A;

enum MacroChar {
    Char(char),
    Modifiers(Modifiers),
}

pub fn write_to_port(port_name: String, config: SerializedConfig) -> Result<()> {
    let mut message = [0u8; 668];

    message[0..4].copy_from_slice(&MAGIC_NUMBER);
    message[6..].copy_from_slice(&config);

    let mut cdc_port = serialport::new(&port_name, 115_200)
        .open()
        .map_err(|err| Error::PortError(err.description))?;

    cdc_port
        .write_all(&message)
        .map_err(|_| Error::PortError("Write to port failed".to_string()))?;

    Ok(())
}

pub fn find_macro_keyboard_port() -> Result<String> {
    find_port(MACRO_KEYBOARD_VID, MACRO_KEYBOARD_PID)
}

fn find_port(vid: u16, pid: u16) -> Result<String>
where
{
    let ports = serialport::available_ports();
    let port_name = ports
        .map_err(|err| Error::PortError(err.description))?
        .into_iter()
        .find_map(|port| {
            if let serialport::SerialPortType::UsbPort(usb_port) = port.port_type {
                if usb_port.pid == pid && usb_port.vid == vid {
                    return Some(port.port_name);
                }
                None
            } else {
                None
            }
        })
        .ok_or(Error::PortError(
            "No suitable device found, make sure the device is on.".to_string(),
        ))?;
    Ok(port_name)
}
pub fn serialize_config(config: &MacroConfig) -> SerializedConfig {
    const HEADER: usize = 2;
    let mut buf = [0 as u8; 662];
    buf[0] = config.general.profiles_num;

    for (chunk, usb_hid_profile) in buf[HEADER..].chunks_exact_mut(220).zip(
        config
            .profiles
            .clone()
            .into_iter()
            .map(serialize_macro_profile),
    ) {
        chunk.copy_from_slice(&usb_hid_profile);
    }

    buf
}

fn serialize_macro_profile(mcr_profile: MacroProfile) -> [u8; 220] {
    let mut buf = [0u8; 220];

    let macros: [Macro; 11] = mcr_profile
        .macros
        .iter()
        .chain([&mcr_profile.encoder.left, &mcr_profile.encoder.right])
        .cloned()
        .collect::<Vec<_>>()
        .try_into()
        .unwrap();

    for (chunk, usb_hid_macro) in buf.chunks_exact_mut(20).zip(macros.map(serialize_macro)) {
        chunk.copy_from_slice(&usb_hid_macro);
    }

    buf
}
fn serialize_macro(mcr: Macro) -> [u8; 20] {
    const HEADER: usize = 4;
    let mut buf = [0u8; 20];
    let macro_len: u8 = mcr.seq.len() as u8;
    let macro_mod: u8 = translate_char_to_usb(MacroChar::Modifiers(mcr.modifiers));
    buf[0] = macro_len;
    buf[1] = macro_mod;

    for (slot, usb_hid) in buf[HEADER..].iter_mut().zip(
        mcr.seq
            .chars()
            .map(|chr| translate_char_to_usb(MacroChar::Char(chr))),
    ) {
        *slot = usb_hid
    }

    buf
}
fn translate_char_to_usb(chr: MacroChar) -> u8 {
    match chr {
        MacroChar::Char(c) => char_to_hid_code(c),
        MacroChar::Modifiers(mdf) => modifiers_to_hid_code(mdf),
    }
}
