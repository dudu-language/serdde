#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

expect_failure() {
    local file="$1"
    local expected="$2"
    local output
    local status

    set +e
    output="$(duc check "$file" 2>&1)"
    status=$?
    set -e

    if [[ $status -eq 0 ]]; then
        printf 'expected %s to fail\n' "$file" >&2
        return 1
    fi
    if [[ "$output" != *"$expected"* ]]; then
        printf 'unexpected diagnostic for %s\nexpected: %s\nactual:\n%s\n' \
            "$file" "$expected" "$output" >&2
        return 1
    fi
    printf 'ok %s\n' "$file"
}

expect_failure tests/compile_fail/variant_duplicate.dd \
    "Serde variant alternatives must have distinct types"
expect_failure tests/compile_fail/duplicate_name.dd \
    "duplicate serialized field name: value"
expect_failure tests/compile_fail/default_conflict.dd \
    "Serde default and default_with cannot be used together"
expect_failure tests/compile_fail/flatten_unknown.dd \
    "flatten cannot be combined with deny_unknown_fields"
expect_failure tests/compile_fail/invalid_rename.dd \
    "unknown Serde rename_all rule: LOUD"
expect_failure tests/compile_fail/skipped_required.dd \
    "skipped deserialization requires a field default"
