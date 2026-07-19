#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
git_ref=""
if [[ "${1:-}" == "--git-ref" ]]; then
    git_ref="${2:?--git-ref requires a commit or tag}"
elif [[ $# -ne 0 ]]; then
    printf 'usage: %s [--git-ref REV]\n' "$0" >&2
    exit 2
fi

workspace="$(mktemp -d "${TMPDIR:-/tmp}/serdde-consumer.XXXXXX")"
trap 'rm -rf "$workspace"' EXIT

run_consumer() {
    local name="$1"
    local dependency="$2"
    local project="$workspace/$name"
    mkdir -p "$project/src"
    cp "$root/tests/package_consumer/main.dd" "$project/src/main.dd"
    printf '%s\n' \
        "name = \"serdde_${name}_consumer\"" \
        "entry = \"src/main.dd\"" \
        "" \
        "[deps]" \
        "serdde = $dependency" >"$project/dudu.toml"
    (cd "$project" && dudu run --quiet)
}

run_consumer path "{ path = \"$root\" }"

if [[ -n "$git_ref" ]]; then
    run_consumer git \
        "{ git = \"https://github.com/dudu-language/serdde.git\", rev = \"$git_ref\" }"
fi
