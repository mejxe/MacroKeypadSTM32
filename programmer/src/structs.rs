use crate::loader;
use serde::Deserialize;

pub type SerializedConfig = [u8; 662];
pub type Modifiers = Option<Vec<Modifier>>;

#[derive(serde::Deserialize, Debug, Clone)]
pub struct Macro {
    pub seq: String,
    pub modifiers: Modifiers,
}

#[repr(u8)]
#[derive(Debug, Deserialize, Clone)]
pub enum Modifier {
    LCTRL = 0x01,
    LSHIFT = 0x02,
    LALT = 0x04,
    LMETA = 0x08,
    RCTRL = 0x10,
    RSHIFT = 0x20,
    RALT = 0x40,
    RMETA = 0x80,
}

#[derive(serde::Deserialize, Debug, Clone)]
pub struct Encoder {
    pub left: Macro,
    pub right: Macro,
}
#[derive(serde::Deserialize, Debug, Clone)]
pub struct MacroProfile {
    #[serde(skip)]
    pub profile_num: u8,
    pub encoder: Encoder,
    #[serde(deserialize_with = "loader::deserialize_macro_map_to_vec")]
    pub macros: Vec<Macro>,
}
#[derive(serde::Deserialize, Debug, Clone)]
pub struct GeneralSettings {
    pub profiles_num: u8,
}
#[derive(serde::Deserialize, Debug)]
pub struct MacroConfig {
    pub general: GeneralSettings,
    #[serde(alias = "profile")]
    pub profiles: Vec<MacroProfile>,
}
