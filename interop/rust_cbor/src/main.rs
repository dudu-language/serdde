use serde::{Deserialize, Serialize};
use std::io::Cursor;

const DUDU_GOLDEN: &str = "a362696407646e616d65634164616673636f72657383020305";

#[derive(Debug, Deserialize, PartialEq, Serialize)]
struct Record {
    id: u64,
    name: String,
    scores: Vec<i32>,
}

fn decode_hex(source: &str) -> Result<Vec<u8>, String> {
    if !source.len().is_multiple_of(2) {
        return Err("hex input has odd length".into());
    }
    source
        .as_bytes()
        .chunks_exact(2)
        .map(|pair| {
            let text = std::str::from_utf8(pair).map_err(|error| error.to_string())?;
            u8::from_str_radix(text, 16).map_err(|error| error.to_string())
        })
        .collect()
}

fn encode_hex(source: &[u8]) -> String {
    source.iter().map(|byte| format!("{byte:02x}")).collect()
}

fn expected_record() -> Record {
    Record {
        id: 7,
        name: "Ada".into(),
        scores: vec![2, 3, 5],
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mode = std::env::args().nth(1).unwrap_or_else(|| "verify".into());
    match mode.as_str() {
        "verify" => {
            let bytes = decode_hex(DUDU_GOLDEN)?;
            let decoded: Record = ciborium::from_reader(Cursor::new(bytes))?;
            if decoded != expected_record() {
                return Err(format!("unexpected Dudu payload: {decoded:?}").into());
            }
        }
        "emit" => {
            let mut bytes = Vec::new();
            ciborium::into_writer(&expected_record(), &mut bytes)?;
            println!("{}", encode_hex(&bytes));
        }
        _ => return Err(format!("unknown mode: {mode}").into()),
    }
    Ok(())
}
