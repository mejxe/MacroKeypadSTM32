use crate::{Error, PortErrorType, Result};

const CRC_MAGIC_NUM: u32 = 0x04c11db7;

pub(crate) fn find_port(vid: u16, pid: u16) -> Result<String> {
    let ports = serialport::available_ports();
    let port_name = ports
        .map_err(|err| Error::PortError(err.description, PortErrorType::Misc))?
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
            PortErrorType::NoPortFound,
        ))?;
    Ok(port_name)
}

pub(crate) fn calculate_crc(bytes: &[u8]) -> u32 {
    let mut crc = 0xFFFF_FFFFu32;
    let (words, rest) = bytes.as_chunks::<4>();
    for word in words {
        crc = calculate_crc_for_word(word, crc);
    }
    if rest.len() > 0 {
        let mut last = [0xffu8; 4];
        last[..rest.len()].copy_from_slice(rest);
        crc = calculate_crc_for_word(&last, crc);
    }

    return crc;
}
fn calculate_crc_for_word(word: &[u8; 4], crc: u32) -> u32 {
    let word_u32 = u32::from_ne_bytes(*word);
    let mut new_crc = crc ^ word_u32;
    for _ in 0..32 {
        if (new_crc & 0x8000_0000) != 0 {
            new_crc = (new_crc << 1) ^ CRC_MAGIC_NUM;
        } else {
            new_crc = new_crc << 1;
        }
    }
    return new_crc;
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn crc_test_vector() {
        let words_9: [u32; 9] = [
            0x00001021, 0x20423063, 0x408450a5, 0x60c670e7, 0x9129a14a, 0xb16bc18c, 0xd1ade1ce,
            0xf1ef1231, 0x32732252,
        ];
        let words_114: [u32; 114] = [
            0x00001021, 0x20423063, 0x408450a5, 0x60c670e7, 0x9129a14a, 0xb16bc18c, 0xd1ade1ce,
            0xf1ef1231, 0x32732252, 0x52b54294, 0x72f762d6, 0x93398318, 0xa35ad3bd, 0xc39cf3ff,
            0xe3de2462, 0x34430420, 0x64e674c7, 0x44a45485, 0xa56ab54b, 0x85289509, 0xf5cfc5ac,
            0xd58d3653, 0x26721611, 0x063076d7, 0x569546b4, 0xb75ba77a, 0x97198738, 0xf7dfe7fe,
            0xc7bc48c4, 0x58e56886, 0x78a70840, 0x18612802, 0xc9ccd9ed, 0xe98ef9af, 0x89489969,
            0xa90ab92b, 0x4ad47ab7, 0x6a961a71, 0x0a503a33, 0x2a12dbfd, 0xfbbfeb9e, 0x9b798b58,
            0xbb3bab1a, 0x6ca67c87, 0x5cc52c22, 0x3c030c60, 0x1c41edae, 0xfd8fcdec, 0xad2abd0b,
            0x8d689d49, 0x7e976eb6, 0x5ed54ef4, 0x2e321e51, 0x0e70ff9f, 0xefbedfdd, 0xcffcbf1b,
            0x9f598f78, 0x918881a9, 0xb1caa1eb, 0xd10cc12d, 0xe16f1080, 0x00a130c2, 0x20e35004,
            0x40257046, 0x83b99398, 0xa3fbb3da, 0xc33dd31c, 0xe37ff35e, 0x129022f3, 0x32d24235,
            0x52146277, 0x7256b5ea, 0x95a88589, 0xf56ee54f, 0xd52cc50d, 0x34e224c3, 0x04817466,
            0x64475424, 0x4405a7db, 0xb7fa8799, 0xe75ff77e, 0xc71dd73c, 0x26d336f2, 0x069116b0,
            0x76764615, 0x5634d94c, 0xc96df90e, 0xe92f99c8, 0xb98aa9ab, 0x58444865, 0x78066827,
            0x18c008e1, 0x28a3cb7d, 0xdb5ceb3f, 0xfb1e8bf9, 0x9bd8abbb, 0x4a755a54, 0x6a377a16,
            0x0af11ad0, 0x2ab33a92, 0xed0fdd6c, 0xcd4dbdaa, 0xad8b9de8, 0x8dc97c26, 0x5c644c45,
            0x3ca22c83, 0x1ce00cc1, 0xef1fff3e, 0xdf7caf9b, 0xbfba8fd9, 0x9ff86e17, 0x7e364e55,
            0x2e933eb2, 0x0ed11ef0,
        ];
        let cases: [(&[u32], u32); 2] = [(&words_9, 0xff813A5c), (&words_114, 0x379e9f06)];

        for (words, expected) in cases {
            let bytes: Vec<u8> = words.iter().flat_map(|w| w.to_ne_bytes()).collect();
            let crc = calculate_crc(&bytes);
            assert_eq!(crc, expected);
        }
    }
}
