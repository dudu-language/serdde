#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
crate="$root/interop/rust_cbor"
expected="a362696407646e616d65634164616673636f72657383020305"

cargo run --quiet --manifest-path "$crate/Cargo.toml" -- verify
actual="$(cargo run --quiet --manifest-path "$crate/Cargo.toml" -- emit)"
if [[ "$actual" != "$expected" ]]; then
    printf 'ciborium emitted:\n%s\nexpected canonical bytes:\n%s\n' \
        "$actual" "$expected" >&2
    exit 1
fi

printf 'CBOR interoperability checks passed (Serdde <-> Rust ciborium)\n'
