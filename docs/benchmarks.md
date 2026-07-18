# JSON Benchmarks

The benchmark compares Serdde's typed JSON API with a handwritten C++ record
using nlohmann/json. It is a representative comparison, not a claim that the
two libraries have identical architecture or optimization priorities.

Both programs:

- use the same five-field record and data;
- serialize the record 50,000 times;
- parse and convert the same JSON document 50,000 times;
- compile as C++20 with `-O3 -DNDEBUG`;
- time work inside the process with `std::chrono::steady_clock`;
- report byte and ID sums so the loops remain observable.

Run:

```bash
./scripts/bench_json.sh
```

Compiler startup and CMake time are excluded. The script keeps this benchmark
outside the normal test loop because it is intentionally heavier than the
conformance fixtures. Results depend on the compiler, standard library, CPU,
and nlohmann/json version and should be recorded with that environment.

## Baseline

Environment:

- AMD Ryzen 9 9950X, 16 cores / 32 threads;
- Ubuntu GCC 13.3.0;
- nlohmann/json 3.11.3;
- Dudu 0.1.0-alpha.13;
- 5 measured process runs after both binaries were built.

Median times:

| implementation | serialize 50,000 | parse and typed decode 50,000 | output bytes |
| --- | ---: | ---: | ---: |
| Dudu / Serdde | 117.34 ms | 196.13 ms | 6,100,000 |
| C++ / nlohmann | 42.34 ms | 51.64 ms | 6,100,000 |

For this fixture Serdde is 2.77x slower while serializing and 3.80x slower
while parsing and converting to the typed record. The benchmark uses Serdde's
general recursive `Value` tree and structured path machinery; it does not yet
have a direct streaming fast path. nlohmann/json is also a convenience-oriented
DOM library, so this is a useful baseline rather than a lower bound for C++ JSON
performance.

Re-run the script before using these values in release notes. The checked-in
numbers describe one machine and one record shape, not every workload.

## Direct-Wire Performance Requirements

The mandatory `Value` path is being replaced according to the
[Direct Wire And Binary Plan](direct-wire-plan.md). The resulting benchmark
suite must report more than elapsed time:

- typed encode and decode latency and throughput;
- allocation count and allocated bytes;
- peak resident memory;
- parser, conversion, and writer time where separable;
- output size, generated C++ size, and compile time;
- small, nested, collection-heavy, and malformed-input workloads.

Equivalent Rust Serde fixtures are mandatory comparison points. JSON uses
`serde_json`; CBOR uses a maintained Serde-compatible implementation. Benchmark
sources remain checked in and record toolchains, dependency versions,
optimization settings, and machine details. Results include cold and warm
compilation cost and binary size as well as runtime, allocation, memory, and
wire-size measurements.

Normal typed operations must perform no intermediate `Value` allocations.
Allocations owned by destination strings and containers remain part of the
reported cost.
