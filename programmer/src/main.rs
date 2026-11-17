use std::env::args;
use std::thread::sleep;
use std::time::Duration;

use macro_uploader::Mode;
use macro_uploader::Result;
use macro_uploader::flashing;
use macro_uploader::loader::{parse_config, validate_config};
use macro_uploader::writer::{find_macro_keyboard_port, serialize_config, write_to_port};

fn main() -> Result<()> {
    let mut cli_args = args();

    cli_args.next();

    let mode: Mode = cli_args.next().expect("No arguments provided").into();
    match mode {
        Mode::MacroMode => macro_mode(),
        Mode::FlashMode => flash_mode(),
    }
}

fn macro_mode() -> Result<()> {
    let config = parse_config("./config.toml");
    validate_config(&config)?;

    let port_name = find_macro_keyboard_port()?;
    let serialized_config = serialize_config(&config);

    write_to_port(port_name, serialized_config)?;
    println!("Macro Keyboard was successfully flashed with your macros!");

    Ok(())
}

fn flash_mode() -> Result<()> {
    let mut bin = flashing::find_bin()?;
    let mut bootloader_port = flashing::find_bootloader_port();

    match bootloader_port {
        Err(macro_uploader::Error::PortError(_, macro_uploader::PortErrorType::NoPortFound)) => {
            let keyboard_port_name = find_macro_keyboard_port()?;
            flashing::send_flash_message(keyboard_port_name)?;

            println!("Waiting for bootloader to connect...");
            sleep(Duration::new(2, 0));

            bootloader_port = flashing::find_bootloader_port();

            flashing::flash(bootloader_port?, &mut bin)?;
        }
        Ok(bp) => {
            flashing::flash(bp, &mut bin)?;
        }
        Err(err) => return Err(err),
    };

    Ok(())
}
