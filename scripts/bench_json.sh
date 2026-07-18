#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build/benchmarks"
mkdir -p "$build"

printf '==> build Dudu/Serdde benchmark\n'
pushd "$root/benchmarks/dudu" >/dev/null
dudu build --quiet
popd >/dev/null

printf '==> build C++/nlohmann benchmark\n'
"${CXX:-c++}" -std=c++20 -O3 -DNDEBUG \
    "$root/benchmarks/cpp_json.cpp" -o "$build/cpp_json"

printf '\n==> Dudu/Serdde\n'
"$root/benchmarks/dudu/build/cmake-backend/build/serdde-json-benchmark"

printf '\n==> C++/nlohmann\n'
"$build/cpp_json"
