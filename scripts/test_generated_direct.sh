#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

expanded="$(mktemp)"
trap 'rm -f "$expanded"' EXIT

for fixture in examples/basic.dd examples/configuration.dd examples/native_adapter.dd; do
    duc expand "$fixture" --show-origins >"$expanded"
    if rg -n '(^|[^A-Za-z0-9_])(Value|ValueKind|to_value|from_value)([^A-Za-z0-9_]|$)|_serdde_convert|serdde\.convert' "$expanded"; then
        printf 'generated codec depends on the dynamic Value backend: %s\n' "$fixture" >&2
        exit 1
    fi
done
