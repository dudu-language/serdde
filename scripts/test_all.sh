#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

printf '==> dudu tests\n'
dudu test --quiet

targets=(tests derive config enums generics recursive variant json dson cbor cbor_features cbor_conformance adapters protocol_probe)
for target in "${targets[@]}"; do
    printf '==> %s\n' "$target"
    dudu run "$target" --quiet
done

printf '==> compile-fail diagnostics\n'
./scripts/test_compile_fail.sh

printf '==> generated direct codecs\n'
./scripts/test_generated_direct.sh

printf '==> examples\n'
for example in examples/*.dd; do
    dudu run "$example" --quiet >/dev/null
done
