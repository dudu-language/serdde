# Serdde

Serdde is format-neutral typed serialization for Dudu. Derive macros generate
ordinary Dudu methods that write directly to, and read directly from, a wire
protocol. Typed JSON, CBOR, and DSON operations do not construct an
intermediate dynamic tree.

```python
from serdde.json import dumps
from serdde.json import loads
from serdde.macros import Serde


@derive(Serde)
class Player:
    id: u64
    name: str
    scores: list[i32]


player = Player(id=7, name="Ada", scores=[2, 3, 5])
encoded = dumps(player)
decoded = loads[Player](encoded.value)
```

`@derive(Serde)` uses Dudu's public macro AST. The Dudu compiler contains no
Serdde registry, format logic, package-name check, or native-library special
case. Generated methods are normal checked Dudu code and can be inspected with
`duc expand`.

## Add The Dependency

From Git:

```toml
[deps]
serdde = { git = "https://github.com/dudu-language/serdde.git", branch = "master" }
```

For local development:

```toml
[deps]
serdde = { path = "../serdde" }
```

`dudu build`, `dudu run`, and `dudu test` fetch missing Git dependencies. The
resolved commit is recorded in `dudu.lock`.

## Formats

| Format | Typed modules | Dynamic module | Intended use |
| --- | --- | --- | --- |
| JSON | `serdde.json` | `serdde.json_value` | Text interchange and APIs |
| CBOR | `serdde.cbor` | `serdde.cbor_value` | Compact deterministic binary interchange |
| DSON | `serdde.dson` | `serdde.dson_value` | Exact typed text for debugging and tests |

Each typed format provides `dumps`, `loads[T]`, `dumps_into`, `dumps_with`,
`loads_with`, and `dumps_into_with`. The `_with` forms apply an explicit adapter
to a standalone value. Field adapters use `@Serde(adapter="AdapterName")`.

Dynamic data is an explicit choice. Import `Value` from `serdde.value`, convert
typed values with `serdde.convert`, or parse/write a tree through a format's
`*_value` module. Dynamic operations use the same serializer/deserializer
protocol as an ordinary backend; they are not underneath typed wire calls.

## Supported Behavior

| Area | Behavior |
| --- | --- |
| Derives | `Serialize`, `Deserialize`, and `Serde` for classes, generic classes, enums, and recursive models |
| Scalars | bool, char, signed and unsigned integers, `usize`, `f32`, `f64`, and `str` |
| Containers | `list`, `set`, string-keyed `dict`, arbitrary-rank fixed arrays, `Option`, `Result`, and `variant` |
| Enums | unit and named-payload variants; external, internal, adjacent, and untagged representations |
| Configuration | rename rules, aliases, defaults, directional skips, predicates, flattening, and unknown-field policy |
| External types | direct field and top-level adapters, including imported C++ templates |
| Errors | stable kinds, field/index paths, format names, and source positions |
| CBOR schemas | deterministic name-keyed maps and explicit stable compact integer field IDs |

Raw pointers and references are not serialized implicitly. Their ownership and
identity require a handwritten codec or adapter.

## Implementation

JSON typed reads use yyjson behind a private reader boundary. Object entries
are indexed once and decoded directly into destination fields. JSON writes use
a small direct writer with checked UTF-8, escaping, integer conversion, and
finite-float policy. simdjson On-Demand, RapidJSON SAX, yyjson, Glaze, and
nlohmann/json are retained in the reproducible candidate matrix. The
production reader/writer choice is based on complete Serdde typed operations,
not parser-only or writer-only microbenchmarks.

CBOR follows RFC 8949. Typed classes use deterministic maps whose default keys
are serialized field names. `@Serde(compact=True)` uses explicit field IDs;
IDs are never derived from declaration order, hashes, native offsets, or ABI
layout. Golden bytes and Rust `ciborium` interoperability are tested.

DSON distinguishes signed integers, unsigned integers, and floats in text. It
implements the same direct protocols and is useful when exact categories need
to be visible.

## Commands

Run the normal local validation suite:

```sh
./scripts/test_all.sh
```

Validate only the standalone CMake entry point:

```sh
./scripts/test_cmake.sh
```

Validate clean path consumption, or an exact published Git revision:

```sh
./scripts/test_package_consumers.sh
./scripts/test_package_consumers.sh --git-ref REV
```

Inspect generated code:

```sh
duc expand examples/basic.dd --show-origins
```

Run all equivalent benchmark smoke cases:

```sh
./scripts/bench_wire.sh --smoke
```

Run and publish the complete benchmark matrix separately:

```sh
./scripts/bench_wire.sh --all --publish
```

The matrix covers Serdde direct, Serdde `Value`, Rust Serde, yyjson, simdjson
On-Demand, RapidJSON SAX, nlohmann/json, and Glaze across JSON and CBOR where
supported. It records latency, throughput, allocations, allocated bytes, peak
RSS, wire size, binary size, generated C++ size, and cold/warm compilation
cost.

## Documentation

- [API](docs/api.md)
- [Architecture](docs/architecture.md)
- [Configuration](docs/configuration.md)
- [Wire formats](docs/dson.md)
- [Schema evolution](docs/schema-evolution.md)
- [Generated code](docs/generated-code.md)
- [Editor behavior](docs/editor.md)
- [Benchmarks](docs/benchmarks.md)
- [Direct-wire design and delivery record](docs/direct-wire-plan.md)

## License

Apache-2.0 OR MIT.
