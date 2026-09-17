use serde::{Deserialize, Deserializer};
use std::fs;
use thiserror::Error;
use std::collections::BTreeMap;

pub type Result<T> = std::result::Result<T, Error>;
#[derive(Debug, Deserialize)]
enum Modifier {
    LCTRL,
    LSHIFT,
    LALT,
    RCTRL,
    RSHIFT,
    RALT,
}

fn main() -> Result<()>{
    let config = parse_config("./config.toml");
    validate_config(&config)?;
    dbg!(&config);
    Ok(())
}
#[derive(Debug, Error)]
pub enum Error {
    #[error("Config error: {0}")]
    ConfigError(String)
}

#[derive(serde::Deserialize, Debug)]
struct Macro {
    seq: String,
    modifier: Option<Modifier>
}

#[derive(serde::Deserialize, Debug)]
struct Encoder {
    left: Macro,
    right: Macro,
}
#[derive(serde::Deserialize, Debug)]
struct MacroProfile {
    #[serde(skip)]
    profile_num: u8,
    encoder: Encoder,
    #[serde(deserialize_with = "deserialize_macro_map_to_vec")]
    macros: Vec<Macro>,
}
#[derive(serde::Deserialize, Debug)]
struct GeneralSettings {
    profiles_num: u8,
}
#[derive(serde::Deserialize, Debug)]
struct MacroConfig {
    general: GeneralSettings,
    #[serde(alias = "profile")]
    profiles: Vec<MacroProfile>
}
fn parse_config(path: &str) -> MacroConfig {
    let read_file = fs::read_to_string(path).expect("should be there");
    let config: MacroConfig = toml::from_str(&read_file).unwrap();
    config
}
fn validate_config(config: &MacroConfig) -> Result<()> {
    match (config.general.profiles_num) {
        1..3 if config.general.profiles_num as usize == config.profiles.len() => (),
        1..3 => 
        return Err(Error::ConfigError(format!("Invalid amount of profiles: profiles_num({}) != declared profiles({})", config.general.profiles_num, config.profiles.len())).into()),
        _ => 
        return Err(Error::ConfigError(format!("Invalid amount of profiles: 1 <= profiles_num({}) <= 3)", config.general.profiles_num)).into()),
    };
    for profile in &config.profiles {
        validate_profile(profile)?;
    }
    Ok(())
}
fn validate_profile(profile: &MacroProfile) -> Result<()> {
    validate_macro(&profile.encoder.left)?;
    validate_macro(&profile.encoder.right)?;
    for mcr in &profile.macros {
        validate_macro(mcr)?;
    }
    Ok(())
}
fn validate_macro(r#macro: &Macro) -> Result<()> {
    if (r#macro.seq.len() > 15) {
        return Err(Error::ConfigError(format!("Macro {} is too long, macro_len({}) > 15", r#macro.seq, r#macro.seq.len())).into());
    }
    Ok(())
}

fn deserialize_macro_map_to_vec<'de, D>(deserializer: D) -> std::result::Result<Vec<Macro>, D::Error> where D: Deserializer<'de> {
    let map: BTreeMap<String, Macro> = BTreeMap::deserialize(deserializer)?;

    let mut pairs: Vec<(usize, Macro)>= map.into_iter()
    .map(|(k, v)| k.parse::<usize>().map(|num| (num, v)))
    .collect::<std::result::Result<_, _>>()
    .map_err(serde::de::Error::custom)?;

    pairs.sort_by_key(|(n, _)| *n);
    Ok(pairs.into_iter().map(|(_,v)|v).collect())
}