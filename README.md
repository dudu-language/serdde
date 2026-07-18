# Serdde

Serdde is a format-neutral serialization framework for Dudu. Derive macros
generate ordinary typed Dudu code for classes and enums. JSON and DSON share
the same conversion layer.

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

Serdde does not require compiler knowledge of the package. `@derive(Serde)`
uses Dudu's public macro AST and emits normal checked Dudu declarations.

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
resolved commit is recorded in `dudu.lock`. `dudu deps fetch` may also be run
explicitly.

Import derives from `serdde.macros` and formats from their own modules:

```python
from serdde.dson import dumps as dumps_dson
from serdde.json import dumps as dumps_json
from serdde.macros import Serde
```

## Features

| Area | Supported behavior |
| --- | --- |
| Derives | `Serialize`, `Deserialize`, and `Serde` for classes, generic classes, enums, and recursive models |
| Scalars | bool, char, signed and unsigned integers, `usize`, `f32`, `f64`, and `str` |
| Containers | `list`, `set`, string-keyed `dict`, arbitrary-rank fixed arrays, `Option`, `Result`, and `variant` |
| Enums | unit and named-payload variants; external, internal, adjacent, and untagged representations |
| Configuration | rename rules, aliases, defaults, directional skips, predicates, flattening, and unknown-field policy |
| External types | explicit field and top-level adapters, including imported C++ templates |
| Errors | stable kinds, typed field/index paths, format names, and parser source locations |
| Formats | strict JSON and deterministic typed DSON |

Raw pointers and references are not serialized implicitly. Their ownership and
identity need an explicit adapter or handwritten codec.

## Commands

Run the normal tests and every executable fixture:

```sh
dudu test
./scripts/test_all.sh
```

Inspect code emitted by a derive:

```sh
duc expand examples/basic.dd --show-origins
```

Run an example:

```sh
dudu run examples/basic.dd
```

Run the heavier JSON comparison separately:

```sh
./scripts/bench_json.sh
```

## Documentation

- [API](docs/api.md)
- [Configuration](docs/configuration.md)
- [Schema evolution](docs/schema-evolution.md)
- [Architecture](docs/architecture.md)
- [JSON and DSON](docs/dson.md)
- [Generated code](docs/generated-code.md)
- [Editor behavior](docs/editor.md)
- [Benchmarks](docs/benchmarks.md)
- [Direct wire, optimized JSON, and CBOR plan](docs/direct-wire-plan.md)

## License

Apache-2.0 OR MIT.
