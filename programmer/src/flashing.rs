use std::{fs::File, io::Read, os::unix::fs::MetadataExt};

use crate::{
    BOOTLOADER_PID, BOOTLOADER_VID, Error, FLASH_FIRMWARE_MESSAGE, MAGIC_NUMBER, PATH,
    PortErrorType, Result, helpers::calculate_crc,
};

pub fn find_bin() -> Result<File> {
    let bin = File::open(PATH)
        .map_err(|_| Error::BinaryError(format!("No matching binary named: {}", PATH)))?;
    Ok(bin)
}
pub fn send_flash_message(port_name: String) -> Result<()> {
    let mut message = [0u8; 5];

    message[0..4].copy_from_slice(&MAGIC_NUMBER);
    message[4] = FLASH_FIRMWARE_MESSAGE;

    let mut cdc_port = serialport::new(&port_name, 115_200)
        .open()
        .map_err(|err| Error::PortError(err.description, PortErrorType::Misc))?;

    cdc_port
        .write_all(&message)
        .map_err(|_| Error::PortError("Write to port failed".to_string(), PortErrorType::Misc))?;

    Ok(())
}

pub fn find_bootloader_port() -> Result<String> {
    crate::helpers::find_port(BOOTLOADER_VID, BOOTLOADER_PID)
}

/*
 * 4B mnumber, 8B size, 4B crc32 hash, 32b padding, size B program
 */
pub fn flash(bootloader_port_name: String, binary: &mut File) -> Result<()> {
    let mut message: Vec<u8> = vec![0; 64];

    message[0..4].copy_from_slice(&MAGIC_NUMBER);

    let size: u64 = binary.metadata().unwrap().size();
    message[4..12].copy_from_slice(&size.to_le_bytes());

    binary
        .read_to_end(&mut message)
        .map_err(|err| Error::BinaryError(err.to_string()))?;

    let crc = calculate_crc(&message[64..]);
    dbg!(&crc);
    message[12..16].copy_from_slice(&crc.to_le_bytes());

    let mut cdc_port = serialport::new(&bootloader_port_name, 115_200)
        .open()
        .map_err(|err| Error::PortError(err.description, PortErrorType::Misc))?;

    dbg!(message.len());
    cdc_port
        .write_all(&message)
        .map_err(|_| Error::PortError("Write to port failed".to_string(), PortErrorType::Misc))?;

    Ok(())
}
