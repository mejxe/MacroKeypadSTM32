pub use thiserror::Error;
pub type Result<T> = std::result::Result<T, Error>;

pub mod loader;
pub mod writer;

mod structs;
mod usb_hid;

#[derive(Debug, Error)]
pub enum Error {
    #[error("Config error: {0}")]
    ConfigError(String),
    #[error("Port error: {0}")]
    PortError(String),
}
