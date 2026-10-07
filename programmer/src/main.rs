use macro_uploader::Result;
use macro_uploader::loader::{parse_config, validate_config};
use macro_uploader::writer::{find_macro_keyboard_port, serialize_config, write_to_port};

fn main() -> Result<()> {
    let config = parse_config("./config.toml");
    validate_config(&config)?;
    let port_name = find_macro_keyboard_port()?;
    let serialized_config = serialize_config(&config);
    write_to_port(port_name, serialized_config)?;
    println!("Macro Keyboard was successfully flashed with your macros!");
    Ok(())
}
