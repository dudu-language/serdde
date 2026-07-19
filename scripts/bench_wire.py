#!/usr/bin/env python3
"""Build and measure equivalent direct-wire JSON and CBOR benchmarks."""

from __future__ import annotations

import argparse
import csv
import json
import os
import platform
import shutil
import statistics
import subprocess
import tempfile
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "benchmarks"
RESULTS = BUILD / "results"
GLAZE_DIR = BUILD / "deps" / "glaze"
GLAZE_REPOSITORY = "https://github.com/stephenberry/glaze.git"
GLAZE_VERSION = "v7.9.1"
GLAZE_COMMIT = "cadcadea26554cc4214769e358f981426e40a02a"
SIMDJSON_DIR = BUILD / "deps" / "simdjson"
SIMDJSON_REPOSITORY = "https://github.com/simdjson/simdjson.git"
SIMDJSON_VERSION = "v4.6.4"
SIMDJSON_COMMIT = "1bcf71bd85059ab6574ea1159de9298dcc1212c5"
RAPIDJSON_DIR = BUILD / "deps" / "rapidjson"
RAPIDJSON_REPOSITORY = "https://github.com/Tencent/rapidjson.git"
RAPIDJSON_VERSION = "master-2026-06-01"
RAPIDJSON_COMMIT = "24b5e7a8b27f42fa16b96fc70aade9106cf7102f"
CPP_BUILD = BUILD / "cpp"
RUST_TARGET = BUILD / "rust"
DUDU_PROJECT = ROOT / "benchmarks" / "dudu"
DUDU_BINARY = DUDU_PROJECT / "build/cmake-backend/build/serdde-wire-benchmark"
RUST_BINARY = RUST_TARGET / "release" / "serdde-rust-benchmark"

WORKLOADS = ("small", "nested", "large_string", "large_array", "map", "enum", "malformed")
FORMATS = ("json", "cbor")
ITERATIONS = {
    "small": 50_000,
    "nested": 50_000,
    "large_string": 1_000,
    "large_array": 1_000,
    "map": 1_500,
    "enum": 50_000,
    "malformed": 50_000,
}


@dataclass(frozen=True)
class Implementation:
    label: str
    binary: Path
    formats: tuple[str, ...]
    implementation_env: str | None = None
    operations: tuple[str, ...] = ("encode", "decode")


def implementations() -> tuple[Implementation, ...]:
    return (
        Implementation("Serdde direct", DUDU_BINARY, FORMATS, "serdde-direct"),
        Implementation("Serdde Value", DUDU_BINARY, FORMATS, "serdde-value"),
        Implementation("Rust Serde", RUST_BINARY, FORMATS),
        Implementation("yyjson", CPP_BUILD / "benchmark_yyjson", ("json",)),
        Implementation("simdjson On-Demand", CPP_BUILD / "benchmark_simdjson",
                       ("json",), operations=("decode",)),
        Implementation("RapidJSON SAX", CPP_BUILD / "benchmark_rapidjson", ("json",)),
        Implementation("nlohmann/json", CPP_BUILD / "benchmark_nlohmann", FORMATS),
        Implementation("Glaze", CPP_BUILD / "benchmark_glaze", FORMATS),
    )


def command_output(command: list[str], cwd: Path = ROOT) -> str:
    return subprocess.check_output(command, cwd=cwd, text=True, stderr=subprocess.STDOUT).strip()


def run(command: list[str], cwd: Path = ROOT, quiet: bool = False,
        env: dict[str, str] | None = None) -> None:
    print("+", " ".join(command), flush=True)
    kwargs: dict[str, Any] = {"cwd": cwd, "env": env, "check": True}
    if quiet:
        kwargs.update(stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    subprocess.run(command, **kwargs)


def ensure_checkout(directory: Path, repository: str, commit: str) -> None:
    if (directory / ".git").is_dir():
        current = command_output(["git", "rev-parse", "HEAD"], directory)
        if current == commit:
            return
        shutil.rmtree(directory)
    directory.parent.mkdir(parents=True, exist_ok=True)
    run(["git", "clone", "--filter=blob:none", "--no-checkout", repository,
         str(directory)])
    run(["git", "fetch", "--depth=1", "origin", commit], directory)
    run(["git", "checkout", "--detach", commit], directory)


def ensure_native_dependencies() -> None:
    ensure_checkout(GLAZE_DIR, GLAZE_REPOSITORY, GLAZE_COMMIT)
    ensure_checkout(SIMDJSON_DIR, SIMDJSON_REPOSITORY, SIMDJSON_COMMIT)
    ensure_checkout(RAPIDJSON_DIR, RAPIDJSON_REPOSITORY, RAPIDJSON_COMMIT)


def cpp_configure_command() -> list[str]:
    return ["cmake", "-S", str(ROOT / "benchmarks/cpp"), "-B", str(CPP_BUILD),
            "-DCMAKE_BUILD_TYPE=Release", f"-DGLAZE_INCLUDE_DIR={GLAZE_DIR}/include",
            f"-DSIMDJSON_DIR={SIMDJSON_DIR}",
            f"-DRAPIDJSON_INCLUDE_DIR={RAPIDJSON_DIR}/include"]


def build_all() -> None:
    ensure_native_dependencies()
    run([os.environ.get("DUDU", "dudu"), "build", "--quiet"], DUDU_PROJECT)
    run(cpp_configure_command())
    run(["cmake", "--build", str(CPP_BUILD), "--parallel"])
    rust_env = os.environ.copy()
    rust_env["CARGO_TARGET_DIR"] = str(RUST_TARGET)
    run(["cargo", "build", "--release", "--locked"], ROOT / "benchmarks/rust", env=rust_env)


def parse_metrics(output: str) -> dict[str, Any]:
    metrics: dict[str, Any] = {}
    for line in output.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        metrics[key] = int(value) if value.lstrip("-").isdigit() else value
    required = {
        "implementation", "format", "workload", "operation", "iterations",
        "elapsed_ns", "wire_bytes", "allocation_count", "allocated_bytes", "checksum",
    }
    missing = required - metrics.keys()
    if missing:
        raise RuntimeError(f"benchmark output omitted {sorted(missing)}:\n{output}")
    return metrics


def measured_process(implementation: Implementation, wire_format: str, workload: str,
                     operation: str, iterations: int) -> dict[str, Any]:
    env = os.environ.copy()
    env.update(
        SERDDE_BENCH_FORMAT=wire_format,
        SERDDE_BENCH_WORKLOAD=workload,
        SERDDE_BENCH_OPERATION=operation,
        SERDDE_BENCH_ITERATIONS=str(iterations),
    )
    if implementation.implementation_env:
        env["SERDDE_BENCH_IMPLEMENTATION"] = implementation.implementation_env
    RESULTS.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=RESULTS, delete=False) as rss_file:
        rss_path = Path(rss_file.name)
    try:
        result = subprocess.run(
            ["/usr/bin/time", "-f", "%M", "-o", str(rss_path), str(implementation.binary)],
            env=env,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        metrics = parse_metrics(result.stdout)
        metrics["peak_rss_kib"] = int(rss_path.read_text().strip())
        metrics["label"] = implementation.label
        metrics["ns_per_operation"] = metrics["elapsed_ns"] / metrics["iterations"]
        metrics["operations_per_second"] = 1e9 / metrics["ns_per_operation"]
        total_mib = metrics["wire_bytes"] * metrics["iterations"] / (1024 * 1024)
        metrics["mib_per_second"] = total_mib / (metrics["elapsed_ns"] / 1e9)
        metrics["allocations_per_operation"] = metrics["allocation_count"] / metrics["iterations"]
        metrics["allocated_bytes_per_operation"] = metrics["allocated_bytes"] / metrics["iterations"]
        return metrics
    finally:
        rss_path.unlink(missing_ok=True)


def median_row(rows: list[dict[str, Any]]) -> dict[str, Any]:
    result = dict(rows[0])
    median_keys = (
        "elapsed_ns", "peak_rss_kib", "ns_per_operation", "operations_per_second",
        "mib_per_second", "allocation_count", "allocated_bytes",
        "allocations_per_operation", "allocated_bytes_per_operation",
    )
    for key in median_keys:
        result[key] = statistics.median(row[key] for row in rows)
    result["samples"] = len(rows)
    return result


def runtime_matrix(repetitions: int, selected_formats: tuple[str, ...],
                   target_ms: int) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    raw: list[dict[str, Any]] = []
    summary: list[dict[str, Any]] = []
    for wire_format in selected_formats:
        for implementation in implementations():
            if wire_format not in implementation.formats:
                continue
            for workload in WORKLOADS:
                operations = ("decode",) if workload == "malformed" else implementation.operations
                for operation in operations:
                    description = f"{implementation.label}: {wire_format}/{workload}/{operation}"
                    print(description, flush=True)
                    calibration = measured_process(
                        implementation, wire_format, workload, operation, 1,
                    )
                    measured_ns = max(1.0, calibration["ns_per_operation"])
                    count = max(
                        1,
                        min(ITERATIONS[workload], int(target_ms * 1_000_000 / measured_ns)),
                    )
                    samples = [
                        measured_process(implementation, wire_format, workload, operation, count)
                        for _ in range(repetitions)
                    ]
                    raw.extend(samples)
                    summary.append(median_row(samples))
    return raw, summary


def timed_command(command: list[str], cwd: Path, env: dict[str, str] | None = None) -> dict[str, float]:
    RESULTS.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=RESULTS, delete=False) as timing_file:
        timing_path = Path(timing_file.name)
    try:
        measured = ["/usr/bin/time", "-f", "%e %M", "-o", str(timing_path), *command]
        subprocess.run(measured, cwd=cwd, env=env, check=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True)
        seconds, rss = timing_path.read_text().split()
        return {"seconds": float(seconds), "peak_rss_kib": float(rss)}
    finally:
        timing_path.unlink(missing_ok=True)


def compile_matrix() -> list[dict[str, Any]]:
    ensure_native_dependencies()
    rows: list[dict[str, Any]] = []

    run([os.environ.get("DUDU", "dudu"), "clean"], DUDU_PROJECT, quiet=True)
    command = [os.environ.get("DUDU", "dudu"), "build", "--quiet"]
    cold = timed_command(command, DUDU_PROJECT)
    warm = timed_command(command, DUDU_PROJECT)
    rows.append({"implementation": "Serdde direct/Value", "cold_seconds": cold["seconds"],
                 "warm_seconds": warm["seconds"], "cold_peak_rss_kib": cold["peak_rss_kib"]})

    if CPP_BUILD.exists():
        shutil.rmtree(CPP_BUILD)
    run(cpp_configure_command(), quiet=True)
    for target, label in (("benchmark_yyjson", "yyjson"),
                          ("benchmark_simdjson", "simdjson On-Demand"),
                          ("benchmark_rapidjson", "RapidJSON SAX"),
                          ("benchmark_nlohmann", "nlohmann/json"),
                          ("benchmark_glaze", "Glaze")):
        run(["cmake", "--build", str(CPP_BUILD), "--target", "clean"], quiet=True)
        command = ["cmake", "--build", str(CPP_BUILD), "--target", target, "--parallel"]
        cold = timed_command(command, ROOT)
        warm = timed_command(command, ROOT)
        rows.append({"implementation": label, "cold_seconds": cold["seconds"],
                     "warm_seconds": warm["seconds"], "cold_peak_rss_kib": cold["peak_rss_kib"]})

    if RUST_TARGET.exists():
        shutil.rmtree(RUST_TARGET)
    rust_env = os.environ.copy()
    rust_env["CARGO_TARGET_DIR"] = str(RUST_TARGET)
    command = ["cargo", "build", "--release", "--locked"]
    cold = timed_command(command, ROOT / "benchmarks/rust", rust_env)
    warm = timed_command(command, ROOT / "benchmarks/rust", rust_env)
    rows.append({"implementation": "Rust Serde", "cold_seconds": cold["seconds"],
                 "warm_seconds": warm["seconds"], "cold_peak_rss_kib": cold["peak_rss_kib"]})
    build_all()
    return rows


def first_line(command: list[str]) -> str:
    try:
        return command_output(command).splitlines()[0]
    except (FileNotFoundError, subprocess.CalledProcessError):
        return "unavailable"


def machine_details() -> dict[str, Any]:
    os_release: dict[str, str] = {}
    for line in Path("/etc/os-release").read_text().splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            os_release[key] = value.strip('"')
    cpu = "unknown"
    for line in Path("/proc/cpuinfo").read_text().splitlines():
        if line.startswith("model name"):
            cpu = line.split(":", 1)[1].strip()
            break
    memory_kib = next(
        int(line.split()[1]) for line in Path("/proc/meminfo").read_text().splitlines()
        if line.startswith("MemTotal:")
    )
    return {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "distribution": os_release.get("PRETTY_NAME", "unknown"),
        "cpu": cpu,
        "logical_cpus": os.cpu_count(),
        "memory_gib": round(memory_kib / 1024 / 1024, 1),
        "cxx": first_line([os.environ.get("CXX", "c++"), "--version"]),
        "rustc": first_line(["rustc", "--version"]),
        "cargo": first_line(["cargo", "--version"]),
        "cmake": first_line(["cmake", "--version"]),
        "dudu": first_line([os.environ.get("DUDU", "dudu"), "--version"]),
        "serdde_commit": command_output(["git", "rev-parse", "HEAD"], ROOT),
        "dudu_commit": command_output(["git", "rev-parse", "HEAD"], ROOT.parent / "dudu"),
        "glaze_version": GLAZE_VERSION,
        "glaze_commit": GLAZE_COMMIT,
        "simdjson_version": SIMDJSON_VERSION,
        "simdjson_commit": SIMDJSON_COMMIT,
        "rapidjson_version": RAPIDJSON_VERSION,
        "rapidjson_commit": RAPIDJSON_COMMIT,
        "serde_version": "1.0.228",
        "serde_json_version": "1.0.150",
        "ciborium_version": "0.2.2",
        "nlohmann_json_version": "3.11.3",
        "yyjson_version": "0.12.0",
        "optimization": "release; C/C++ -O3 -DNDEBUG; Rust release without LTO",
    }


def artifact_rows() -> list[dict[str, Any]]:
    rows = []
    for implementation in implementations():
        rows.append({"implementation": implementation.label,
                     "binary_bytes": implementation.binary.stat().st_size})
    generated = DUDU_PROJECT / "build/cmake-backend/build/generated"
    files = [path for path in generated.rglob("*") if path.suffix in {".cpp", ".hpp"}]
    rows.append({"implementation": "Dudu generated C++", "binary_bytes": sum(path.stat().st_size for path in files),
                 "lines": sum(len(path.read_text(errors="replace").splitlines()) for path in files)})
    return rows


def write_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    if not rows:
        return
    keys = list(dict.fromkeys(key for row in rows for key in row))
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=keys)
        writer.writeheader()
        writer.writerows(rows)


def markdown_report(metadata: dict[str, Any], summary: list[dict[str, Any]],
                    compile_rows: list[dict[str, Any]], artifacts: list[dict[str, Any]]) -> str:
    lines = [
        "# Direct-Wire Benchmarks", "",
        "This suite compares equivalent typed schemas and operations. Serdde direct, Serdde's optional `Value`",
        "backend, Rust Serde, yyjson, simdjson On-Demand, RapidJSON SAX, nlohmann/json, and Glaze",
        "are measured in separate processes. simdjson is decode-only because it does not provide a JSON writer.", "",
        "## Environment", "",
        f"- Machine: {metadata['cpu']}, {metadata['logical_cpus']} logical CPUs, {metadata['memory_gib']} GiB RAM",
        f"- OS: {metadata['distribution']}", f"- C++: {metadata['cxx']}", f"- Rust: {metadata['rustc']}",
        f"- Dudu: {metadata['dudu']}", f"- Optimization: {metadata['optimization']}",
        f"- Glaze: {metadata['glaze_version']} (`{metadata['glaze_commit']}`)",
        f"- simdjson: {metadata['simdjson_version']} (`{metadata['simdjson_commit']}`)",
        f"- RapidJSON: {metadata['rapidjson_version']} (`{metadata['rapidjson_commit']}`)",
        f"- serde / serde_json / ciborium: {metadata['serde_version']} / {metadata['serde_json_version']} / {metadata['ciborium_version']}",
        f"- nlohmann/json / yyjson: {metadata['nlohmann_json_version']} / {metadata['yyjson_version']}", "",
        "Runtime values are medians of independent process runs after one calibration run. Each case is calibrated",
        "independently toward the requested measurement duration and capped by the checked-in iteration limits.",
        "Allocation counters record gross",
        "allocation requests, not retained bytes. Peak RSS includes process and linked-library startup. Throughput",
        "uses encoded wire bytes. Malformed cases measure rejection and appear separately.", "",
    ]
    for wire_format in FORMATS:
        selected = [row for row in summary if row["format"] == wire_format and row["workload"] != "malformed"]
        if not selected:
            continue
        lines += [f"## {wire_format.upper()}", "",
                  "| implementation | workload | operation | ns/op | MiB/s | alloc/op | allocated B/op | RSS MiB | wire B |",
                  "| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
        for row in selected:
            lines.append(
                f"| {row['label']} | {row['workload']} | {row['operation']} | "
                f"{row['ns_per_operation']:.1f} | {row['mib_per_second']:.1f} | "
                f"{row['allocations_per_operation']:.2f} | {row['allocated_bytes_per_operation']:.1f} | "
                f"{row['peak_rss_kib'] / 1024:.1f} | {row['wire_bytes']} |"
            )
        malformed = [row for row in summary if row["format"] == wire_format and row["workload"] == "malformed"]
        lines += ["", "Malformed-input rejection:", "",
                  "| implementation | ns/rejection | alloc/rejection | allocated B/rejection |",
                  "| --- | ---: | ---: | ---: |"]
        for row in malformed:
            lines.append(f"| {row['label']} | {row['ns_per_operation']:.1f} | "
                         f"{row['allocations_per_operation']:.2f} | {row['allocated_bytes_per_operation']:.1f} |")
        lines.append("")
    if compile_rows:
        lines += ["## Compilation", "", "| implementation | cold seconds | warm seconds | cold RSS MiB |",
                  "| --- | ---: | ---: | ---: |"]
        for row in compile_rows:
            lines.append(f"| {row['implementation']} | {row['cold_seconds']:.2f} | {row['warm_seconds']:.2f} | "
                         f"{row['cold_peak_rss_kib'] / 1024:.1f} |")
        lines.append("")
    lines += ["## Artifact Size", "", "| artifact | bytes | generated lines |", "| --- | ---: | ---: |"]
    for row in artifacts:
        lines.append(f"| {row['implementation']} | {row['binary_bytes']} | {row.get('lines', '')} |")
    lines += ["", "## Reproduce", "", "```bash", "./scripts/bench_wire.sh --all --publish", "```", "",
              "The runner fetches pinned Glaze, simdjson, and RapidJSON commits into ignored build storage. Rust",
              "dependencies are fixed by `benchmarks/rust/Cargo.lock`; yyjson is vendored; system nlohmann/json is",
              "version-checked by CMake.", ""]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--runtime", action="store_true", help="run only runtime measurements")
    modes.add_argument("--compile", action="store_true", help="run only cold/warm compilation measurements")
    modes.add_argument("--smoke", action="store_true", help="run every supported case once")
    modes.add_argument("--all", action="store_true", help="run runtime and compilation measurements")
    parser.add_argument("--format", choices=FORMATS, help="limit runtime cases to one format")
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--target-ms", type=int, default=100,
                        help="calibrated duration of each runtime sample")
    parser.add_argument("--publish", action="store_true", help="replace docs/benchmarks.md with measured results")
    args = parser.parse_args()
    if args.repetitions < 1:
        parser.error("--repetitions must be positive")
    if args.target_ms < 1:
        parser.error("--target-ms must be positive")

    selected_formats = (args.format,) if args.format else FORMATS
    run_runtime = args.runtime or args.all or not args.compile
    run_compile = args.compile or args.all
    if args.smoke or run_runtime:
        build_all()
    else:
        ensure_native_dependencies()
    if args.smoke:
        expected_checksums: dict[tuple[str, str], int] = {}
        for wire_format in selected_formats:
            for implementation in implementations():
                if wire_format not in implementation.formats:
                    continue
                for workload in WORKLOADS:
                    operations = ("decode",) if workload == "malformed" else implementation.operations
                    for operation in operations:
                        metrics = measured_process(
                            implementation, wire_format, workload, operation, 1)
                        if operation == "decode":
                            key = (wire_format, workload)
                            expected = expected_checksums.setdefault(key, metrics["checksum"])
                            if metrics["checksum"] != expected:
                                raise RuntimeError(
                                    f"{implementation.label} decoded {key} to checksum "
                                    f"{metrics['checksum']}, expected {expected}")
        print("All benchmark smoke cases passed")
        return 0

    previous: dict[str, Any] = {}
    result_path = RESULTS / "benchmark-results.json"
    if result_path.exists():
        previous = json.loads(result_path.read_text())
    raw: list[dict[str, Any]] = []
    summary: list[dict[str, Any]] = list(previous.get("runtime", []))
    compile_rows: list[dict[str, Any]] = list(previous.get("compilation", []))
    if run_runtime:
        raw, measured = runtime_matrix(args.repetitions, selected_formats, args.target_ms)
        summary = [row for row in summary if row.get("format") not in selected_formats]
        summary.extend(measured)
    if run_compile:
        compile_rows = compile_matrix()
    metadata = machine_details()
    artifacts = artifact_rows()
    RESULTS.mkdir(parents=True, exist_ok=True)
    payload = {"metadata": metadata, "runtime": summary, "compilation": compile_rows, "artifacts": artifacts}
    result_path.write_text(json.dumps(payload, indent=2) + "\n")
    if raw:
        write_csv(RESULTS / "benchmark-runs.csv", raw)
    write_csv(RESULTS / "benchmark-summary.csv", summary)
    write_csv(RESULTS / "compile-summary.csv", compile_rows)
    report = markdown_report(metadata, summary, compile_rows, artifacts)
    (RESULTS / "benchmark-results.md").write_text(report)
    if args.publish:
        (ROOT / "docs/benchmarks.md").write_text(report)
    print(f"Results: {RESULTS / 'benchmark-results.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
